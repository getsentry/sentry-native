#include "../modulefinder.hpp"

#ifdef SENTRY_WITH_LINUX_MODULEFINDER
#include <arpa/inet.h>
#include <elf.h>
#include <link.h>
#include <unistd.h>
#include <mutex>
#include "../uuid.hpp"

using namespace sentry;
using namespace modulefinders;

static Value g_modules;
static std::mutex g_modules_mutex;
static bool g_initialized = false;

void align(size_t alignment, size_t *offset) {
    size_t diff = *offset % alignment;
    if (diff != 0) {
        *offset += alignment - diff;
    }
}

int dl_iterate_callback(struct dl_phdr_info *dl_info, size_t size, void *data) {
    if (dl_info->dlpi_phnum <= 0) {
        return 0;
    }

    Value module = Value::new_object();
    uint64_t image_addr = (uint64_t)-1;
    uint64_t image_end_addr = 0;
    bool have_build_id = false;

    for (size_t i = 0; i < dl_info->dlpi_phnum; i++) {
        if (dl_info->dlpi_phdr[i].p_type == PT_LOAD) {
            uint64_t start = dl_info->dlpi_addr + dl_info->dlpi_phdr[i].p_vaddr;
            uint64_t end = start + dl_info->dlpi_phdr[i].p_memsz;
            if (start < image_addr) {
                image_addr = start;
            }
            if (end > image_end_addr) {
                image_end_addr = end;
            }
        } else if (dl_info->dlpi_phdr[i].p_type == PT_NOTE && !have_build_id) {
            size_t alignment = (size_t)dl_info->dlpi_phdr[i].p_align;
            if (alignment < 4) {
                alignment = 4;
            } else if (alignment != 4 && alignment != 8) {
                continue;
            }

            size_t offset = (size_t)dl_info->dlpi_phdr[i].p_vaddr;
            size_t end = offset + (size_t)dl_info->dlpi_phdr[i].p_filesz;

            while (offset < end) {
                const Elf32_Nhdr *nhdr =
                    (const Elf32_Nhdr *)((char *)dl_info->dlpi_addr + offset);
                offset += sizeof(Elf32_Nhdr);
                offset += (size_t)nhdr->n_namesz;
                align(alignment, &offset);
                char *note = (char *)dl_info->dlpi_addr + offset;
                offset += (size_t)nhdr->n_descsz;
                align(alignment, &offset);
                if (nhdr->n_type == NT_GNU_BUILD_ID) {
                    module.set_by_key(
                        "code_id", Value::new_hexstring(note, nhdr->n_descsz));
                    sentry_uuid_t uuid = sentry_uuid_from_bytes(note);

                    char *uuid_bytes = (char *)&uuid.native_uuid;
                    uint32_t *a = (uint32_t *)&uuid_bytes;
                    *a = htonl(*a);
                    uint16_t *b = (uint16_t *)&uuid_bytes + 4;
                    *b = htons(*b);
                    uint16_t *c = (uint16_t *)&uuid_bytes + 6;
                    *c = htons(*c);

                    module.set_by_key("debug_id", Value::new_uuid(&uuid));
                    have_build_id = true;
                    break;
                }
            }
        }
    }

    module.set_by_key("type", Value::new_string("elf"));
    module.set_by_key("image_addr", Value::new_addr(image_addr));
    module.set_by_key("image_size",
                      Value::new_int32((int32_t)(image_end_addr - image_addr)));

    if (!dl_info->dlpi_name || !*dl_info->dlpi_name) {
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
        if (len >= 0) {
            module.set_by_key("code_file",
                              Value::new_string(path, (size_t)len));
        }
    } else {
        module.set_by_key("code_file", Value::new_string(dl_info->dlpi_name));
    }
    g_modules.append(module);

    return 0;
}

Value modulefinders::get_module_list() {
    std::lock_guard<std::mutex> _guard(g_modules_mutex);
    if (!g_initialized) {
        g_modules = Value::new_list();
        dl_iterate_phdr(dl_iterate_callback, nullptr);
        g_initialized = true;
    }
    return g_modules;
}

#endif
