/* SwiftPM configuration for Linux x86_64 and aarch64, with glibc or musl. */
#if !defined(__linux__) || (!defined(__x86_64__) && !defined(__aarch64__))
#    error "SentryUnwind requires Linux x86_64 or aarch64"
#endif

/* Enable Debug Frame */
/* #undef CONFIG_DEBUG_FRAME */

/* Block signals before mutex operations */
/* #undef CONFIG_BLOCK_SIGNALS */

/* Support for Microsoft ABI extensions */
/* #undef CONFIG_MSABI_SUPPORT */

/* Define if the weak 'backtrace' symbol is provided. */
/* #undef CONFIG_WEAK_BACKTRACE */

/* Define to 1 if you want every memory access validated */
/* #undef CONSERVATIVE_CHECKS */

#define HAVE_ASM_PTRACE_H 1
/* #undef HAVE_ASM_PTRACE_OFFSETS_H */
/* #undef HAVE_ASM_VSYSCALL_H */
#define HAVE_BYTESWAP_H 1

#define HAVE_DECL_PTRACE_CONT 1
#define HAVE_DECL_PTRACE_POKEDATA 1
#define HAVE_DECL_PTRACE_POKEUSER 1
#define HAVE_DECL_PTRACE_SETREGSET 1
#define HAVE_DECL_PTRACE_SINGLESTEP 1
#define HAVE_DECL_PTRACE_SYSCALL 1
#define HAVE_DECL_PTRACE_TRACEME 1

#define HAVE_DECL_PT_CONTINUE 0
#define HAVE_DECL_PT_GETFPREGS 0
#define HAVE_DECL_PT_GETREGS 0
#define HAVE_DECL_PT_IO 0
#define HAVE_DECL_PT_STEP 0
#define HAVE_DECL_PT_SYSCALL 0
#define HAVE_DECL_PT_TRACE_ME 0

/* #undef HAVE_DLFCN_H */
/* #undef HAVE_DLMODINFO */
#define HAVE_DL_ITERATE_PHDR 1
/* #undef HAVE_DL_PHDR_REMOVALS_COUNTER */
/* #undef HAVE_ELF_FPREGSET_T */
#define HAVE_ELF_H 1
#define HAVE_ENDIAN_H 1
#if __has_include(<execinfo.h>)
#    define HAVE_EXECINFO_H 1
#endif
/* #undef HAVE_EXECVPE */
/* #undef HAVE_GETUNWIND */
/* #undef HAVE_IA64INTRIN_H */
#define HAVE_INTTYPES_H 1
#define HAVE_LINK_H 1
/* #undef HAVE_LZMA */
#define HAVE_MINCORE 1
#define HAVE_PIPE2 1
/* #undef HAVE_PROCFS_STATUS */
#define HAVE_SIGALTSTACK 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRUCT_DL_PHDR_INFO_DLPI_SUBS 1
/* #undef HAVE_STRUCT_ELF_PRSTATUS */
/* #undef HAVE_STRUCT_PRSTATUS */
/* #undef HAVE_SYS_ELF_H */
/* #undef HAVE_SYS_ENDIAN_H */
/* #undef HAVE_SYS_LINK_H */
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_PROCFS_H 1
#define HAVE_SYS_PTRACE_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_SYSCALL_H 1
#define HAVE_SYS_TYPES_H 1
/* #undef HAVE_SYS_UC_ACCESS_H */
#define HAVE_UNISTD_H 1
/* #undef HAVE_ZLIB */

#define PACKAGE_STRING "libunwind 1.8.3"
#define PACKAGE_BUGREPORT "N/A"

#define STDC_HEADERS 1

/* size of off_t type */
#define SIZEOF_OFF_T 8
