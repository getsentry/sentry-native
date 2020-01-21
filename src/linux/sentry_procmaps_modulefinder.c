#include "sentry_procmaps_modulefinder.h"
#include "../sentry_core.h"
#include "../sentry_path.h"
#include "../sentry_string.h"
#include "../sentry_sync.h"
#include "../sentry_value.h"
#include <arpa/inet.h>
#include <elf.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

static bool g_initialized = false;
static sentry_mutex_t g_mutex = SENTRY__MUTEX_INIT;
static sentry_value_t g_modules;

int
sentry__procmaps_parse_module_line(char *line, sentry_module_t *module)
{
    char permissions[5] = { 0 };
    uint64_t offset;
    uint8_t major_device;
    uint8_t minor_device;
    uint64_t inode;
    int consumed = 0;

    // this has been copied from the breakpad source:
    // https://github.com/google/breakpad/blob/13c1568702e8804bc3ebcfbb435a2786a3e335cf/src/processor/proc_maps_linux.cc#L66
    if (sscanf(line,
            "%" SCNx64 "-%" SCNx64 " %4c %" SCNx64 " %hhx:%hhx %" SCNd64 " %n",
            &module->start, &module->end, permissions, &offset, &major_device,
            &minor_device, &inode, &consumed)
        < 7) {
        return 0;
    }

    // copy the filename up to a newline
    line += consumed;
    char *nl = strchr(line, '\n');
    if (consumed && (line - 1)[0] == '\n') {
        module->file = NULL;
    } else if (nl) {
        module->file = sentry__string_clonen(line, nl - line);
        consumed += nl - line + 1;
    } else {
        module->file = sentry__string_clone(line);
        consumed += strlen(line);
    }

    // and return the consumed chars…
    return consumed;
}

void
align(size_t alignment, const char **offset)
{
    size_t diff = (size_t)*offset % alignment;
    if (diff != 0) {
        *offset += alignment - diff;
    }
}

static char *
get_debug_id_from_notes(size_t alignment, const char *offset, const char *end)
{
    if (alignment < 4) {
        alignment = 4;
    } else if (alignment != 4 && alignment != 8) {
        return NULL;
    }

    while (offset < end) {
        // The note header size is independant of the architecture, so we just
        // use the `Elf64_Nhdr` variant.
        const Elf64_Nhdr *note = (const Elf64_Nhdr *)offset;
        // the headers are consecutive, and the optional `name` and `desc` are
        // saved inline after the header.

        offset += sizeof(Elf64_Nhdr);
        offset += note->n_namesz;
        align(alignment, &offset);
        const char *description = offset;
        if (note->n_type == NT_GNU_BUILD_ID) {
            return sentry__string_clonen(description, note->n_descsz);
        }
        offset += note->n_descsz;
        align(alignment, &offset);
    }
    return NULL;
}

static bool
is_elf_module(const char *addr)
{
    // we try to interpret `addr` as an ELF file, which should start with a
    // magic number…
    const unsigned char *e_ident = (const unsigned char *)addr;
    return e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1
        && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3;
}

static char *
get_debug_id_from_elf(const char *addr)
{
    const unsigned char *e_ident = (const unsigned char *)addr;
    // iterate over all the program headers, for 32/64 bit separately
    if (e_ident[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *elf = (const Elf64_Ehdr *)addr;
        for (int i = 0; i < elf->e_phnum; i++) {
            const Elf64_Phdr *header = (const Elf64_Phdr *)(addr + elf->e_phoff
                + elf->e_phentsize * i);
            // we are only interested in notes
            if (header->p_type != PT_NOTE) {
                continue;
            }
            return get_debug_id_from_notes(header->p_align,
                (const char *)(addr + header->p_vaddr),
                (const char *)(addr + header->p_vaddr + header->p_memsz));
        }
    } else {
        const Elf32_Ehdr *elf = (const Elf32_Ehdr *)addr;
        for (int i = 0; i < elf->e_phnum; i++) {
            const Elf64_Phdr *header = (const Elf64_Phdr *)(addr + elf->e_phoff
                + elf->e_phentsize * i);
            // we are only interested in notes
            if (header->p_type != PT_NOTE) {
                continue;
            }
            return get_debug_id_from_notes(header->p_align,
                (const char *)(addr + header->p_vaddr),
                (const char *)(addr + header->p_vaddr + header->p_memsz));
        }
    }
    return NULL;
}

static sentry_value_t
module_to_value(const sentry_module_t *module)
{
    sentry_value_t mod_val = sentry_value_new_object();
    sentry_value_set_by_key(mod_val, "type", sentry_value_new_string("elf"));
    sentry_value_set_by_key(
        mod_val, "image_addr", sentry__value_new_addr((uint64_t)module->start));
    sentry_value_set_by_key(mod_val, "image_size",
        sentry_value_new_int32((int32_t)(module->end - module->start)));
    sentry_value_set_by_key(
        mod_val, "code_file", sentry__value_new_string_owned(module->file));

    // and try to get the debug id from the elf headers of the loaded
    // modules
    char *debug_id = get_debug_id_from_elf((const char *)module->start);
    if (debug_id) {

        // the usage of these is described here:
        // https://getsentry.github.io/symbolicator/advanced/symbol-server-compatibility/#identifiers
        // in particular, the debug_id is a `little-endian GUID`, so we have to
        // do appropriate byte-flipping
        sentry_value_t code_id
            = sentry__value_new_hexstring(debug_id, strlen(debug_id));
        sentry_value_set_by_key(mod_val, "code_id", code_id);

        sentry_uuid_t uuid = sentry_uuid_from_bytes(debug_id);
        char *uuid_bytes = (char *)uuid.bytes;
        uint32_t *a = (uint32_t *)uuid_bytes;
        *a = htonl(*a);
        uint16_t *b = (uint16_t *)(uuid_bytes + 4);
        *b = htons(*b);
        uint16_t *c = (uint16_t *)(uuid_bytes + 6);
        *c = htons(*c);

        sentry_value_set_by_key(
            mod_val, "debug_id", sentry__value_new_uuid(&uuid));
    }

    return mod_val;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void
try_append_module(sentry_value_t *modules, const sentry_module_t *module)
{
    if (!is_elf_module((const char *)module->start)) {
        return;
    }

    if (!module->file) {
        return;
    }
    sentry_value_append(*modules, module_to_value(module));
}

static void
load_modules(void)
{
    g_modules = sentry_value_new_list();

    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) {
        goto done;
    }

    // just read the whole map at once, maybe do it line-by-line as a followup…
    char buf[4096];
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    while (1) {
        ssize_t n = read(fd, buf, 4096);
        if (n <= 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            break;
        }
        sentry__stringbuilder_append_buf(&sb, buf, n);
    }
    close(fd);

    char *contents = sentry__stringbuilder_into_string(&sb);
    char *current_line = contents;

    // we have multiple memory maps per file, and we need to merge their offsets
    // based on the filename. Luckily, the maps are ordered by filename, so yay
    sentry_module_t last_module = { SIZE_MAX, 0, NULL };
    while (true) {
        sentry_module_t module = { 0, 0, NULL };
        int read = sentry__procmaps_parse_module_line(current_line, &module);
        current_line += read;

        if (!read) {
            break;
        }
        // skip over anonymous mappings
        if (!module.file
            || (module.file[0] == '[' && strcmp("[vdso]", module.file) != 0)) {
            continue;
        }

        if (last_module.file && strcmp(last_module.file, module.file) != 0) {
            try_append_module(&g_modules, &last_module);
            last_module = module;
        } else {
            // otherwise merge it
            last_module.start = MIN(last_module.start, module.start);
            last_module.end = MAX(last_module.end, module.end);
            sentry_free(last_module.file);
            last_module.file = module.file;
        }
    }
    try_append_module(&g_modules, &last_module);
    sentry_free(contents);

done:

    SENTRY_TRACEF("read %zu modules from /proc/self/maps",
        sentry_value_get_length(g_modules));

    sentry_value_freeze(g_modules);
}

sentry_value_t
sentry__procmaps_modules_get_list(void)
{
    sentry__mutex_lock(&g_mutex);
    if (!g_initialized) {
        load_modules();
        g_initialized = true;
    }
    sentry__mutex_unlock(&g_mutex);
    return g_modules;
}
