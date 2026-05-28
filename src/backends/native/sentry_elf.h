#ifndef SENTRY_ELF_H_INCLUDED
#define SENTRY_ELF_H_INCLUDED

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)

#    include <elf.h>
#    include <stdbool.h>
#    include <stddef.h>

static inline bool
sentry__elf_is_native_class(const unsigned char e_ident[EI_NIDENT])
{
#    if defined(__x86_64__) || defined(__aarch64__)
    return e_ident[EI_CLASS] == ELFCLASS64;
#    else
    return e_ident[EI_CLASS] == ELFCLASS32;
#    endif
}

static inline bool
sentry__elf_has_shdr_size(
    const unsigned char e_ident[EI_NIDENT], size_t e_shentsize)
{
    if (!sentry__elf_is_native_class(e_ident)) {
        return false;
    }

#    if defined(__x86_64__) || defined(__aarch64__)
    return e_shentsize == sizeof(Elf64_Shdr);
#    else
    return e_shentsize == sizeof(Elf32_Shdr);
#    endif
}

static inline bool
sentry__elf_has_sym_entsize(
    const unsigned char e_ident[EI_NIDENT], size_t sh_entsize)
{
    if (!sentry__elf_is_native_class(e_ident)) {
        return false;
    }

#    if defined(__x86_64__) || defined(__aarch64__)
    return sh_entsize == sizeof(Elf64_Sym);
#    else
    return sh_entsize == sizeof(Elf32_Sym);
#    endif
}

static inline bool
sentry__elf_has_phdr_size(
    const unsigned char e_ident[EI_NIDENT], size_t e_phentsize)
{
    if (!sentry__elf_is_native_class(e_ident)) {
        return false;
    }

#    if defined(__x86_64__) || defined(__aarch64__)
    return e_phentsize == sizeof(Elf64_Phdr);
#    else
    return e_phentsize == sizeof(Elf32_Phdr);
#    endif
}

#endif

#endif
