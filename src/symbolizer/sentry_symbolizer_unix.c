#include "sentry_boot.h"

#include "sentry_symbolizer.h"

#include <dlfcn.h>
#include <string.h>

/* XXX: Break into a separate file */
#ifdef SENTRY_PLATFORM_AIX
/*
 * AIX doesn't have dladdr and we must go through hoops instead. What we'll do
 * instead is reimplement most of what dladdr does from what loader information
 * AIX provides us. The following code is ripped from mono, but is under the
 * same license, and I am also the author of it.
 */

#    include <limits.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

/* AIX specific headers for loadquery and traceback structure */
#    include <sys/debug.h>
#    include <sys/ldr.h>

/* library filename + ( + member file name + ) + NUL */
#    define AIX_PRINTED_LIB_LEN ((PATH_MAX * 2) + 3)

/*
 * The structure that holds information for dladdr. Unfortunately, on AIX,
 * the information returned by loadquery lives in an allocated buffer, so it
 * should be freed when no longer needed. Note that sname /is/ still constant
 * (it points to the traceback info in the image), so don't free it.
 */
typedef struct dl_info {
    // these aren't const* because they are allocated
    char *dli_fname;
    void *dli_fbase;
    char *dli_sname;
    void *dli_saddr;
} Dl_info;

static const void *
tb_consume(const char **ptr, const void *end, size_t size)
{
    uintptr_t ptr_addr = (uintptr_t)*ptr;
    uintptr_t end_addr = (uintptr_t)end;
    if (ptr_addr > end_addr || size > end_addr - ptr_addr) {
        return NULL;
    }
    const void *rv = *ptr;
    *ptr += size;
    return rv;
}

/**
 * Gets the base address and name of a symbol.
 *
 * This uses the traceback table at the function epilogue to get the base
 * address and the name of a symbol. As such, this means that the input must
 * be a word-aligned address within the text section.
 *
 * The way to support non-text (data/bss/whatever) would be to use an XCOFF
 * parser on the image loaded in memory and snarf its symbol table. However,
 * that is much more complex, and presumably, most addresses passed would be
 * code in the text section anyways (I hope so, anyways...) Unfortunately,
 * this does mean that function descriptors, which live in data, won't work.
 * The traceback approach actually works with JITted code too, provided it
 * could be emitted with XCOFF traceback...
 */
static void
sym_from_tb(void **sbase, char **sname, void *where, const void *text_end)
{
    /* The pointer must be word aligned as instructions are */
    const char *cur = (const char *)((uintptr_t)where & ~3);
    const unsigned int *s;
    while ((s = tb_consume(&cur, text_end, sizeof(*s))) && *s) {
        /* look for zero word (invalid op) that begins epilogue */
    }
    if (!s) {
        return;
    }
    /* We're on a zero word now, seek after the traceback table. */
    const struct tbtable_short *tb = tb_consume(&cur, text_end, sizeof(*tb));
    if (!tb) {
        return;
    }
    /* The extended traceback is variable length, so more seeking. */
    /* Skip a lot of cruft, in order according to the ext "structure". */
    if ((tb->fixedparms || tb->floatparms)
        && !tb_consume(&cur, text_end, sizeof(unsigned int))) {
        return;
    }
    if (tb->has_tboff) {
        /* tb_offset */
        const unsigned int *tb_offset
            = tb_consume(&cur, text_end, sizeof(*tb_offset));
        if (!tb_offset) {
            return;
        }
        void *start = (char *)s - *tb_offset;
        *sbase = (void *)start;
    } else {
        /*
         * Can we go backwards instead until we hit a null word,
         * that /precedes/ the block of code?
         * Does the XCOFF/traceback format allow for that?
         */
        *sbase = NULL; /* NULL base address as a sentinel */
    }
    if (tb->int_hndl && !tb_consume(&cur, text_end, sizeof(int))) {
        return;
    }
    if (tb->has_ctl) {
        /* array */
        const int *ctlnum = tb_consume(&cur, text_end, sizeof(*ctlnum));
        if (!ctlnum || *ctlnum < 0 || (size_t)*ctlnum > SIZE_MAX / sizeof(int)
            || !tb_consume(&cur, text_end, (size_t)*ctlnum * sizeof(int))) {
            return;
        }
    }
    if (tb->name_present) {
        /* Oops! It does seem these can contain a null! */
        const unsigned short *name_len
            = tb_consume(&cur, text_end, sizeof(*name_len));
        if (!name_len) {
            return;
        }
        const char *name_start = tb_consume(&cur, text_end, *name_len);
        if (!name_start) {
            return;
        }
        char *name = sentry_malloc((size_t)*name_len + 1);
        if (name) {
            memcpy(name, name_start, *name_len);
            name[*name_len] = '\0';
        }
        *sname = name;
    } else {
        *sname = NULL;
    }
}

/**
 * Look for the base address and name of both a symbol and the corresponding
 * executable in memory. This is a simplistic reimplementation for AIX.
 *
 * Returns 0 on failure and 1 on success. "s" is the address of the symbol,
 * and "i" points to a Dl_info structure to fill. Note that i.dli_fname is
 * not const, and should be freed.
 */
static int
dladdr(void *s, Dl_info *i)
{
    char buf[10000];
    i->dli_fbase = NULL;
    i->dli_fname = NULL;
    i->dli_saddr = NULL;
    i->dli_sname = NULL;
    int r = loadquery(L_GETINFO, buf, 10000);
    if (r == -1) {
        return 0;
    }
    /* The loader info structures are also a linked list. */
    struct ld_info *cur = (struct ld_info *)buf;
    while (1) {
        /*
         * Check in text and data sections. Function descriptors are
         * stored in the data section.
         */
        char *db = (char *)cur->ldinfo_dataorg;
        char *tb = (char *)cur->ldinfo_textorg;
        char *de = db + cur->ldinfo_datasize;
        char *te = tb + cur->ldinfo_textsize;
        /* Just casting for comparisons. */
        char *cs = (char *)s;

        /*
         * Find the symbol's name and base address. To make it
         * easier, we use the traceback in the text section.
         * See the function's comments above as to why.
         * (Perhaps we could deref if a descriptor though...)
         */
        if (cs >= tb && cs < te) {
            sym_from_tb(&i->dli_saddr, &i->dli_sname, s, te);
        }

        if ((cs >= db && cs <= de) || (cs >= tb && cs <= te)) {
            /* Look for file name and base address. */
            i->dli_fbase = tb; /* Includes XCOFF header */
            /* library filename + ( + member + ) + NUL */
            char *libname = (char *)sentry_malloc(AIX_PRINTED_LIB_LEN);
            if (!libname) {
                return 0;
            }
            char *file_part = cur->ldinfo_filename;
            char *member_part = file_part + strlen(file_part) + 1;
            /*
             * This can't be a const char*, because it exists from
             * a stack allocated buffer. Also append the member.
             *
             * XXX: See if we can't frob usla's memory ranges for
             * const strings; but is quite difficult.
             */
            if (member_part[0] == '\0') {
                /* Not an archive, just copy the file name. */
                snprintf(libname, AIX_PRINTED_LIB_LEN, "%s", file_part);
            } else {
                /* It's an archive with member. */
                snprintf(libname, AIX_PRINTED_LIB_LEN, "%s(%s)", file_part,
                    member_part);
            }
            i->dli_fname = libname;

            return 1;
        } else if (cur->ldinfo_next == 0) {
            /* Nothing. */
            return 0;
        } else {
            /* Try the next image in memory. */
            cur = (struct ld_info *)((char *)cur + cur->ldinfo_next);
        }
    }
}
#endif

bool
sentry__symbolize(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
{
    Dl_info info;

    if (dladdr(addr, &info) == 0) {
        return false;
    }

    sentry_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(sentry_frame_info_t));
    frame_info.load_addr = info.dli_fbase;
    frame_info.symbol_addr = info.dli_saddr;
    frame_info.instruction_addr = addr;
    frame_info.symbol = info.dli_sname;
    frame_info.object_name = info.dli_fname;
    func(&frame_info, data);
#ifdef SENTRY_PLATFORM_AIX
    // On AIX these must be freed. Hope the callback doesn't use that
    // buffer...
    // XXX: We may just be able to stuff it into a fixed-length field of
    // Dl_info?
    free(info.dli_sname);
    free(info.dli_fname);
#endif

    return true;
}
