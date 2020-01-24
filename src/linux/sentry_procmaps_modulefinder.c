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
#include <sys/auxv.h>
#include <unistd.h>

static bool g_initialized = false;
static sentry_mutex_t g_mutex = SENTRY__MUTEX_INIT;
static sentry_value_t g_modules;

int
sentry__procmaps_parse_module_line(const char *line, sentry_module_t *module)
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
            "%" SCNxPTR "-%" SCNxPTR " %4c %" SCNx64 " %hhx:%hhx %" SCNd64
            " %n",
            (uintptr_t *)&module->start, (uintptr_t *)&module->end, permissions,
            &offset, &major_device, &minor_device, &inode, &consumed)
        < 7) {
        return 0;
    }

    // copy the filename up to a newline
    line += consumed;
    module->file.ptr = line;
    module->file.len = 0;
    char *nl = strchr(line, '\n');
    // `consumed` skips over whitespace (the trailing newline), so we have to
    // check for that explicitly
    if (consumed && (line - 1)[0] == '\n') {
        module->file.ptr = NULL;
    } else if (nl) {
        module->file.len = nl - line;
        consumed += nl - line + 1;
    } else {
        module->file.len = strlen(line);
        consumed += module->file.len;
    }

    // and return the consumed chars…
    return consumed;
}

void
align(size_t alignment, void **offset)
{
    size_t diff = (size_t)*offset % alignment;
    if (diff != 0) {
        *(size_t *)offset += alignment - diff;
    }
}

static const uint8_t *
get_code_id_from_notes(
    size_t alignment, void *start, void *end, size_t *size_out)
{
    *size_out = 0;
    if (alignment < 4) {
        alignment = 4;
    } else if (alignment != 4 && alignment != 8) {
        return NULL;
    }

    const uint8_t *offset = start;
    while (offset < (const uint8_t *)end) {
        // The note header size is independant of the architecture, so we just
        // use the `Elf64_Nhdr` variant.
        const Elf64_Nhdr *note = (const Elf64_Nhdr *)offset;
        // the headers are consecutive, and the optional `name` and `desc` are
        // saved inline after the header.

        offset += sizeof(Elf64_Nhdr);
        offset += note->n_namesz;
        align(alignment, (void **)&offset);
        if (note->n_type == NT_GNU_BUILD_ID) {
            *size_out = note->n_descsz;
            return offset;
        }
        offset += note->n_descsz;
        align(alignment, (void **)&offset);
    }
    return NULL;
}

static bool
is_elf_module(void *addr)
{
    // we try to interpret `addr` as an ELF file, which should start with a
    // magic number…
    const unsigned char *e_ident = addr;
    return e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1
        && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3;
}

static const uint8_t *
get_code_id_from_elf(void *base, size_t *size_out)
{
    *size_out = 0;
    const uint8_t *addr = base;
    const unsigned char *e_ident = addr;
    // iterate over all the program headers, for 32/64 bit separately
    if (e_ident[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *elf = base;
        for (int i = 0; i < elf->e_phnum; i++) {
            const Elf64_Phdr *header = (const Elf64_Phdr *)(addr + elf->e_phoff
                + elf->e_phentsize * i);
            // we are only interested in notes
            if (header->p_type != PT_NOTE) {
                continue;
            }
            const uint8_t *code_id = get_code_id_from_notes(header->p_align,
                (void *)(addr + header->p_vaddr),
                (void *)(addr + header->p_vaddr + header->p_memsz), size_out);
            if (code_id) {
                return code_id;
            }
        }
    } else {
        const Elf32_Ehdr *elf = base;
        for (int i = 0; i < elf->e_phnum; i++) {
            const Elf32_Phdr *header = (const Elf32_Phdr *)(addr + elf->e_phoff
                + elf->e_phentsize * i);
            // we are only interested in notes
            if (header->p_type != PT_NOTE) {
                continue;
            }
            const uint8_t *code_id = get_code_id_from_notes(header->p_align,
                (void *)(addr + header->p_vaddr),
                (void *)(addr + header->p_vaddr + header->p_memsz), size_out);
            if (code_id) {
                return code_id;
            }
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
    sentry_value_set_by_key(mod_val, "code_file",
        sentry__value_new_string_owned(sentry__slice_to_owned(module->file)));

    // and try to get the debug id from the elf headers of the loaded
    // modules
    size_t code_id_size;
    const uint8_t *code_id = get_code_id_from_elf(module->start, &code_id_size);
    if (code_id) {
        // the usage of these is described here:
        // https://getsentry.github.io/symbolicator/advanced/symbol-server-compatibility/#identifiers
        // in particular, the debug_id is a `little-endian GUID`, so we have to
        // do appropriate byte-flipping
        sentry_value_set_by_key(mod_val, "code_id",
            sentry__value_new_hexstring((const char *)code_id, code_id_size));

        sentry_uuid_t uuid = sentry_uuid_from_bytes((const char *)code_id);
        char *uuid_bytes = uuid.bytes;
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
try_append_module(sentry_value_t modules, const sentry_module_t *module)
{
    if (!module->file.ptr || !is_elf_module(module->start)) {
        return;
    }

    sentry_value_append(modules, module_to_value(module));
}

static sentry_slice_t LINUX_GATE = { "linux-gate.so", 13 };

static void
load_modules(sentry_value_t modules)
{
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) {
        return;
    }

    // just read the whole map at once, maybe do it line-by-line as a followup…
    char buf[4096];
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    while (true) {
        ssize_t n = read(fd, buf, 4096);
        if (n <= 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            break;
        }
        if (sentry__stringbuilder_append_buf(&sb, buf, n)) {
            close(fd);
            return;
        }
    }
    close(fd);

    char *contents = sentry__stringbuilder_into_string(&sb);
    if (!contents) {
        return;
    }
    char *current_line = contents;

    // See http://man7.org/linux/man-pages/man7/vdso.7.html
    void *linux_vdso = (void *)getauxval(AT_SYSINFO_EHDR);

    // we have multiple memory maps per file, and we need to merge their offsets
    // based on the filename. Luckily, the maps are ordered by filename, so yay
    sentry_module_t last_module = { (void *)SIZE_MAX, 0, { NULL, 0 } };
    while (true) {
        sentry_module_t module = { 0, 0, { NULL, 0 } };
        int read = sentry__procmaps_parse_module_line(current_line, &module);
        current_line += read;
        if (!read) {
            break;
        }

        // for the vdso, we use the special filename `linux-gate.so`
        if (module.start == linux_vdso) {
            module.file = LINUX_GATE;
        }
        // skip over anonymous mappings
        if (!module.file.len || module.file.ptr[0] == '[') {
            continue;
        }

        if (last_module.file.len
            && sentry__slice_cmp(last_module.file, module.file) != 0) {
            try_append_module(modules, &last_module);
            last_module = module;
        } else {
            // otherwise merge it
            last_module.start = MIN(last_module.start, module.start);
            last_module.end = MAX(last_module.end, module.end);
            last_module.file = module.file;
        }
    }
    try_append_module(modules, &last_module);
    sentry_free(contents);
}

sentry_value_t
sentry__procmaps_modules_get_list(void)
{
    sentry__mutex_lock(&g_mutex);
    if (!g_initialized) {
        g_modules = sentry_value_new_list();
        load_modules(g_modules);
        SENTRY_TRACEF("read %zu modules from /proc/self/maps",
            sentry_value_get_length(g_modules));
        sentry_value_freeze(g_modules);
        g_initialized = true;
    }
    sentry__mutex_unlock(&g_mutex);
    return g_modules;
}
