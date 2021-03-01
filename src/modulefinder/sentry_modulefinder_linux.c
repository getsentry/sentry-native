#include "sentry_modulefinder_linux.h"

#include "sentry_core.h"
#include "sentry_modulefinder_elf.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_value.h"

#include <arpa/inet.h>
#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static bool g_initialized = false;
static sentry_mutex_t g_mutex = SENTRY__MUTEX_INIT;
static sentry_value_t g_modules = { 0 };

static sentry_slice_t LINUX_GATE = { "linux-gate.so", 13 };

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
            "%" SCNxPTR "-%" SCNxPTR " %4c %" SCNx64 " %hhx:%hhx %" SCNu64
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

bool
sentry__mmap_file(sentry_mmap_t *rv, const char *path)
{
    rv->fd = open(path, O_RDONLY);
    if (rv->fd < 0) {
        goto fail;
    }

    struct stat sb;
    if (stat(path, &sb) != 0 || !S_ISREG(sb.st_mode)) {
        goto fail;
    }

    rv->len = sb.st_size;
    if (rv->len == 0) {
        goto fail;
    }

    rv->ptr = mmap(NULL, rv->len, PROT_READ, MAP_PRIVATE, rv->fd, 0);
    if (rv->ptr == MAP_FAILED) {
        goto fail;
    }

    return true;

fail:
    if (rv->fd > 0) {
        close(rv->fd);
    }
    rv->fd = 0;
    rv->ptr = NULL;
    rv->len = 0;
    return false;

    return rv;
}

void
sentry__mmap_close(sentry_mmap_t *m)
{
    munmap(m->ptr, m->len);
    close(m->fd);
    m->ptr = NULL;
    m->len = 0;
    m->fd = 0;
}

static bool
is_elf_module(void *addr)
{
    // we try to interpret `addr` as an ELF file, which should start with a
    // magic number...
    const unsigned char *e_ident = addr;
    return e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1
        && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3;
}

sentry_value_t
sentry__procmaps_module_to_value(const sentry_module_t *module)
{
    sentry_value_t mod_val = sentry_value_new_object();
    sentry_value_set_by_key(mod_val, "type", sentry_value_new_string("elf"));
    sentry_value_set_by_key(mod_val, "image_addr",
        sentry__value_new_addr((uint64_t)(size_t)module->start));
    sentry_value_set_by_key(mod_val, "image_size",
        sentry_value_new_int32(
            (int32_t)((size_t)module->end - (size_t)module->start)));
    sentry_value_set_by_key(mod_val, "code_file",
        sentry__value_new_string_owned(sentry__slice_to_owned(module->file)));

    // At least on the android API-16, x86 simulator, the linker apparently
    // does not load the complete file into memory. Or at least, the section
    // headers which are located at the end of the file are not loaded, and
    // we would be poking into invalid memory. To be safe, we mmap the complete
    // file from disk, so we have the on-disk layout, and are independent of how
    // the runtime linker would load or re-order any sections. The exception
    // here is the linux-gate, which is not an actual file on disk, so we
    // actually poke at its memory.
    if (sentry__slice_eq(module->file, LINUX_GATE)) {
        if (!is_elf_module(module->start)) {
            goto fail;
        }
        sentry__procmaps_read_ids_from_elf(mod_val, module->start);
    } else {
        char *filename = sentry__slice_to_owned(module->file);
        sentry_mmap_t mm;
        if (!sentry__mmap_file(&mm, filename)) {
            sentry_free(filename);
            goto fail;
        }
        sentry_free(filename);

        if (!is_elf_module(mm.ptr)) {
            sentry__mmap_close(&mm);
            goto fail;
        }

        sentry__procmaps_read_ids_from_elf(mod_val, mm.ptr);

        sentry__mmap_close(&mm);
    }

    return mod_val;

fail:
    sentry_value_decref(mod_val);
    return sentry_value_new_null();
}

static void
try_append_module(sentry_value_t modules, const sentry_module_t *module)
{
    if (!module->file.ptr) {
        return;
    }

    SENTRY_TRACEF(
        "inspecting module \"%.*s\"", (int)module->file.len, module->file.ptr);
    sentry_value_t mod_val = sentry__procmaps_module_to_value(module);
    if (!sentry_value_is_null(mod_val)) {
        sentry_value_append(modules, mod_val);
    }
}

// copied from:
// https://github.com/google/breakpad/blob/216cea7bca53fa441a3ee0d0f5fd339a3a894224/src/client/linux/minidump_writer/linux_dumper.h#L61-L70
#if defined(__i386) || defined(__ARM_EABI__)                                   \
    || (defined(__mips__) && _MIPS_SIM == _ABIO32)
typedef Elf32_auxv_t elf_aux_entry;
#elif defined(__x86_64) || defined(__aarch64__)                                \
    || (defined(__mips__) && _MIPS_SIM != _ABIO32)
typedef Elf64_auxv_t elf_aux_entry;
#endif

// See http://man7.org/linux/man-pages/man7/vdso.7.html
static void *
get_linux_vdso(void)
{
    // this is adapted from:
    // https://github.com/google/breakpad/blob/79ba6a494fb2097b39f76fe6a4b4b4f407e32a02/src/client/linux/minidump_writer/linux_dumper.cc#L548-L572

    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd < 0) {
        return false;
    }

    elf_aux_entry one_aux_entry;
    while (
        read(fd, &one_aux_entry, sizeof(elf_aux_entry)) == sizeof(elf_aux_entry)
        && one_aux_entry.a_type != AT_NULL) {
        if (one_aux_entry.a_type == AT_SYSINFO_EHDR) {
            close(fd);
            return (void *)one_aux_entry.a_un.a_val;
        }
    }
    close(fd);
    return NULL;
}

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
        if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
            continue;
        } else if (n <= 0) {
            break;
        }
        if (sentry__stringbuilder_append_buf(&sb, buf, n)) {
            sentry__stringbuilder_cleanup(&sb);
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

    void *linux_vdso = get_linux_vdso();

    // we have multiple memory maps per file, and we need to merge their offsets
    // based on the filename. Luckily, the maps are ordered by filename, so yay
    sentry_module_t last_module = { (void *)-1, 0, { NULL, 0 } };
    while (true) {
        sentry_module_t module = { 0, 0, { NULL, 0 } };
        int read = sentry__procmaps_parse_module_line(current_line, &module);
        current_line += read;
        if (!read) {
            break;
        }

        // for the vdso, we use the special filename `linux-gate.so`,
        // otherwise we check that we have a valid pathname (with a `/` inside),
        // and skip over things that end in `)`, because entries marked as
        // `(deleted)` might crash when dereferencing, trying to check if its
        // a valid elf file.
        char *slash;
        if (module.start && module.start == linux_vdso) {
            module.file = LINUX_GATE;
        } else if (!module.start || !module.file.len
            || module.file.ptr[module.file.len - 1] == ')'
            || (slash = strchr(module.file.ptr, '/')) == NULL
            || slash > module.file.ptr + module.file.len
            || (module.file.len >= 5
                && memcmp("/dev/", module.file.ptr, 5) == 0)) {
            continue;
        }

        if (last_module.file.len
            && !sentry__slice_eq(last_module.file, module.file)) {
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
sentry_get_modules_list(void)
{
    sentry__mutex_lock(&g_mutex);
    if (!g_initialized) {
        g_modules = sentry_value_new_list();
        SENTRY_TRACE("trying to read modules from /proc/self/maps");
        load_modules(g_modules);
        SENTRY_TRACEF("read %zu modules from /proc/self/maps",
            sentry_value_get_length(g_modules));
        sentry_value_freeze(g_modules);
        g_initialized = true;
    }
    sentry_value_t modules = g_modules;
    sentry_value_incref(modules);
    sentry__mutex_unlock(&g_mutex);
    return modules;
}

void
sentry_clear_modulecache(void)
{
    sentry__mutex_lock(&g_mutex);
    sentry_value_decref(g_modules);
    g_modules = sentry_value_new_null();
    g_initialized = false;
    sentry__mutex_unlock(&g_mutex);
}
