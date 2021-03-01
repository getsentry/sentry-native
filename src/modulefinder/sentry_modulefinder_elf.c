#include "sentry_modulefinder_elf.h"
#include "sentry_core.h"
#include "sentry_string.h"
#include "sentry_value.h"

#include <elf.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

static const uint8_t *
get_code_id_from_elf(void *base, size_t *size_out)
{
    *size_out = 0;

    // now this is interesting:
    // `p_offset` is defined as the offset of the section relative to the file,
    // and `p_vaddr` is supposed to be the memory location.
    // interestingly though, when compiled with gcc 7.4, both are the same,
    // because apparently it does not really patch up the `p_vaddr`. gcc 5.4
    // however does, so `p_vaddr` is an actual pointer, and not an offset to
    // be added to the `base`. So we are using `p_offset` here, since it seems
    // to be the correct offset relative to `base` using both compilers.
    const uint8_t *addr = base;

    // iterate over all the program headers, for 32/64 bit separately
    const unsigned char *e_ident = addr;
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
                (void *)(addr + header->p_offset),
                (void *)(addr + header->p_offset + header->p_memsz), size_out);
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
                (void *)(addr + header->p_offset),
                (void *)(addr + header->p_offset + header->p_memsz), size_out);
            if (code_id) {
                return code_id;
            }
        }
    }
    return NULL;
}

static sentry_uuid_t
get_code_id_from_text_fallback(void *base)
{
    const uint8_t *text = NULL;
    size_t text_size = 0;

    const uint8_t *addr = base;
    // iterate over all the program headers, for 32/64 bit separately
    const unsigned char *e_ident = addr;
    if (e_ident[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *elf = base;
        const Elf64_Shdr *strheader = (const Elf64_Shdr *)(addr + elf->e_shoff
            + elf->e_shentsize * elf->e_shstrndx);

        const char *names = (const char *)(addr + strheader->sh_offset);
        for (int i = 0; i < elf->e_shnum; i++) {
            const Elf64_Shdr *header = (const Elf64_Shdr *)(addr + elf->e_shoff
                + elf->e_shentsize * i);
            const char *name = names + header->sh_name;
            if (header->sh_type == SHT_PROGBITS && strcmp(name, ".text") == 0) {
                text = addr + header->sh_offset;
                text_size = header->sh_size;
                break;
            }
        }
    } else {
        const Elf32_Ehdr *elf = base;
        const Elf32_Shdr *strheader = (const Elf32_Shdr *)(addr + elf->e_shoff
            + elf->e_shentsize * elf->e_shstrndx);

        const char *names = (const char *)(addr + strheader->sh_offset);
        for (int i = 0; i < elf->e_shnum; i++) {
            const Elf32_Shdr *header = (const Elf32_Shdr *)(addr + elf->e_shoff
                + elf->e_shentsize * i);
            const char *name = names + header->sh_name;
            if (header->sh_type == SHT_PROGBITS && strcmp(name, ".text") == 0) {
                text = addr + header->sh_offset;
                text_size = header->sh_size;
                break;
            }
        }
    }

    sentry_uuid_t uuid = sentry_uuid_nil();

    // adapted from
    // https://github.com/getsentry/symbolic/blob/8f9a01756e48dcbba2e42917a064f495d74058b7/debuginfo/src/elf.rs#L100-L110
    size_t max = MIN(text_size, 4096);
    for (size_t i = 0; i < max; i++) {
        uuid.bytes[i % 16] ^= text[i];
    }

    return uuid;
}

bool
sentry__procmaps_read_ids_from_elf(sentry_value_t value, void *elf_ptr)
{
    // and try to get the debug id from the elf headers of the loaded
    // modules
    size_t code_id_size;
    const uint8_t *code_id = get_code_id_from_elf(elf_ptr, &code_id_size);
    sentry_uuid_t uuid = sentry_uuid_nil();
    if (code_id) {
        sentry_value_set_by_key(value, "code_id",
            sentry__value_new_hexstring(code_id, code_id_size));

        memcpy(uuid.bytes, code_id, MIN(code_id_size, 16));
    } else {
        uuid = get_code_id_from_text_fallback(elf_ptr);
    }

    // the usage of these is described here:
    // https://getsentry.github.io/symbolicator/advanced/symbol-server-compatibility/#identifiers
    // in particular, the debug_id is a `little-endian GUID`, so we
    // have to do appropriate byte-flipping
    char *uuid_bytes = uuid.bytes;
    uint32_t *a = (uint32_t *)uuid_bytes;
    *a = htonl(*a);
    uint16_t *b = (uint16_t *)(uuid_bytes + 4);
    *b = htons(*b);
    uint16_t *c = (uint16_t *)(uuid_bytes + 6);
    *c = htons(*c);

    sentry_value_set_by_key(value, "debug_id", sentry__value_new_uuid(&uuid));
    return true;
}
