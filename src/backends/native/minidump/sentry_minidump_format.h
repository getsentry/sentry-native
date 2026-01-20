#ifndef SENTRY_MINIDUMP_FORMAT_H_INCLUDED
#define SENTRY_MINIDUMP_FORMAT_H_INCLUDED

#include <stdint.h>

/**
 * Minidump file format structures
 * Based on Microsoft's minidump format specification
 */

// Define PACKED macro for cross-compiler struct packing
#ifdef _MSC_VER
#    define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
#    define PACKED_STRUCT_END __pragma(pack(pop))
#    define PACKED_ATTR
#    define PACKED_ALIGNED_ATTR(n) __declspec(align(n))
#else
#    define PACKED_STRUCT_BEGIN
#    define PACKED_STRUCT_END
#    define PACKED_ATTR __attribute__((packed))
#    define PACKED_ALIGNED_ATTR(n) __attribute__((packed, aligned(n)))
#endif

#define MINIDUMP_SIGNATURE 0x504d444d // "MDMP"
#define MINIDUMP_VERSION 0xa793

// Stream types
typedef enum {
    MINIDUMP_STREAM_THREAD_LIST = 3,
    MINIDUMP_STREAM_MODULE_LIST = 4,
    MINIDUMP_STREAM_MEMORY_LIST = 5,
    MINIDUMP_STREAM_EXCEPTION = 6,
    MINIDUMP_STREAM_SYSTEM_INFO = 7,
    MINIDUMP_STREAM_THREAD_EX_LIST = 8,
    MINIDUMP_STREAM_MEMORY64_LIST = 9,
    MINIDUMP_STREAM_LINUX_CPU_INFO = 0x47670003,
    MINIDUMP_STREAM_LINUX_PROC_STATUS = 0x47670004,
    MINIDUMP_STREAM_LINUX_MAPS = 0x47670008,
} minidump_stream_type_t;

// CPU types (MINIDUMP_PROCESSOR_ARCHITECTURE)
typedef enum {
    MINIDUMP_CPU_X86 = 0, // PROCESSOR_ARCHITECTURE_INTEL
    MINIDUMP_CPU_ARM = 5, // PROCESSOR_ARCHITECTURE_ARM
    MINIDUMP_CPU_X86_64 = 9, // PROCESSOR_ARCHITECTURE_AMD64
    MINIDUMP_CPU_ARM64 = 12, // PROCESSOR_ARCHITECTURE_ARM64
} minidump_cpu_type_t;

// OS types
typedef enum {
    MINIDUMP_OS_LINUX = 0x8000,
    MINIDUMP_OS_ANDROID = 0x8001,
    MINIDUMP_OS_MACOS = 0x8002,
    MINIDUMP_OS_IOS = 0x8003,
    MINIDUMP_OS_WINDOWS = 2,
} minidump_os_type_t;

/**
 * Minidump RVA (Relative Virtual Address)
 * Offset from start of minidump file
 */
typedef uint32_t minidump_rva_t;

/**
 * Minidump header (always at offset 0)
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t signature; // Must be MINIDUMP_SIGNATURE
    uint32_t version; // Must be MINIDUMP_VERSION
    uint32_t stream_count;
    minidump_rva_t stream_directory_rva;
    uint32_t checksum;
    uint32_t time_date_stamp; // Unix timestamp
    uint64_t flags;
} PACKED_ATTR minidump_header_t;
PACKED_STRUCT_END

/**
 * Stream directory entry
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t stream_type;
    uint32_t data_size;
    minidump_rva_t rva;
} PACKED_ATTR minidump_directory_t;
PACKED_STRUCT_END

/**
 * Location descriptor (used for variable-length data)
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t size;
    minidump_rva_t rva;
} PACKED_ATTR minidump_location_t;
PACKED_STRUCT_END

/**
 * Memory descriptor
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t start_address;
    minidump_location_t memory;
} PACKED_ATTR minidump_memory_descriptor_t;
PACKED_STRUCT_END

/**
 * Memory64 descriptor (more compact for large memory dumps)
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t start_address;
    uint64_t size;
} PACKED_ATTR minidump_memory64_descriptor_t;
PACKED_STRUCT_END

/**
 * Memory list
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t count;
    minidump_memory_descriptor_t ranges[]; // Variable length
} PACKED_ATTR minidump_memory_list_t;
PACKED_STRUCT_END

/**
 * Memory64 list (includes base RVA for all memory)
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t count;
    uint64_t base_rva; // RVA64 per minidump spec - all memory starts here
    minidump_memory64_descriptor_t ranges[]; // Variable length
} PACKED_ATTR minidump_memory64_list_t;
PACKED_STRUCT_END

/**
 * Thread context (CPU state)
 * This is platform-specific and varies by architecture
 */
#if defined(__x86_64__)
// 128-bit value for XMM/FP registers
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t low;
    uint64_t high;
} PACKED_ATTR m128a_t;
PACKED_STRUCT_END

// x87 FPU and SSE/XMM state (512 bytes)
PACKED_STRUCT_BEGIN
typedef struct {
    uint16_t control_word;
    uint16_t status_word;
    uint8_t tag_word;
    uint8_t reserved1;
    uint16_t error_opcode;
    uint32_t error_offset;
    uint16_t error_selector;
    uint16_t reserved2;
    uint32_t data_offset;
    uint16_t data_selector;
    uint16_t reserved3;
    uint32_t mx_csr;
    uint32_t mx_csr_mask;
    m128a_t float_registers[8]; // ST0-ST7 (x87 FPU registers)
    m128a_t xmm_registers[16]; // XMM0-XMM15 (SSE registers)
    uint8_t reserved4[96];
} PACKED_ATTR xmm_save_area32_t;
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t p1_home;
    uint64_t p2_home;
    uint64_t p3_home;
    uint64_t p4_home;
    uint64_t p5_home;
    uint64_t p6_home;
    uint32_t context_flags;
    uint32_t mx_csr;
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
    uint16_t ss;
    uint32_t eflags;
    uint64_t dr0;
    uint64_t dr1;
    uint64_t dr2;
    uint64_t dr3;
    uint64_t dr6;
    uint64_t dr7;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    xmm_save_area32_t float_save; // FPU and XMM state (512 bytes)
    m128a_t vector_register[26]; // AVX extension registers
    uint64_t vector_control;
    uint64_t debug_control;
    uint64_t last_branch_to_rip;
    uint64_t last_branch_from_rip;
    uint64_t last_exception_to_rip;
    uint64_t last_exception_from_rip;
} PACKED_ATTR minidump_context_x86_64_t;
PACKED_STRUCT_END

#elif defined(__aarch64__)
// 128-bit value for NEON registers
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t low;
    uint64_t high;
} PACKED_ATTR uint128_struct;
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t context_flags;
    uint32_t cpsr;
    uint64_t regs[29]; // X0-X28
    uint64_t fp; // X29 (frame pointer)
    uint64_t lr; // X30 (link register)
    uint64_t sp; // Stack pointer
    uint64_t pc; // Program counter
    uint128_struct fpsimd[32]; // NEON/FP registers V0-V31
    uint32_t fpsr; // Floating-point status register
    uint32_t fpcr; // Floating-point control register
    uint32_t bcr[8]; // Debug breakpoint control registers
    uint64_t bvr[8]; // Debug breakpoint value registers
    uint32_t wcr[2]; // Debug watchpoint control registers
    uint64_t wvr[2]; // Debug watchpoint value registers
} PACKED_ATTR minidump_context_arm64_t;
PACKED_STRUCT_END

#elif defined(__i386__)
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t context_flags;
    uint32_t dr0;
    uint32_t dr1;
    uint32_t dr2;
    uint32_t dr3;
    uint32_t dr6;
    uint32_t dr7;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ebp;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} PACKED_ATTR minidump_context_x86_t;
PACKED_STRUCT_END

#elif defined(__arm__)
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t context_flags;
    uint32_t r[13]; // R0-R12
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t cpsr;
} PACKED_ATTR minidump_context_arm_t;
PACKED_STRUCT_END
#endif

/**
 * Thread descriptor
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t thread_id;
    uint32_t suspend_count;
    uint32_t priority_class;
    uint32_t priority;
    uint64_t teb; // Thread Environment Block
    minidump_memory_descriptor_t stack;
    minidump_location_t thread_context;
} PACKED_ATTR minidump_thread_t;
PACKED_STRUCT_END

/**
 * Thread list
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t count;
    minidump_thread_t threads[]; // Variable length
} PACKED_ATTR minidump_thread_list_t;
PACKED_STRUCT_END

/**
 * CPU information union (varies by architecture)
 */
PACKED_STRUCT_BEGIN
typedef union {
    // For x86/x86_64 (when processor_architecture is X86 or AMD64)
    struct {
        uint32_t vendor_id[3]; // cpuid 0: ebx, edx, ecx
        uint32_t version_information; // cpuid 1: eax
        uint32_t feature_information; // cpuid 1: edx
        uint32_t amd_extended_cpu_features; // cpuid 0x80000001: edx
    } PACKED_ALIGNED_ATTR(4) x86_cpu_info;

    // For all other architectures (ARM, ARM64, etc.)
    struct {
        uint64_t processor_features[2]; // Feature flags
    } PACKED_ALIGNED_ATTR(4) other_cpu_info;
} PACKED_ALIGNED_ATTR(4) minidump_cpu_information_t;
PACKED_STRUCT_END

/**
 * System info
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint16_t processor_architecture;
    uint16_t processor_level;
    uint16_t processor_revision;
    uint8_t number_of_processors;
    uint8_t product_type;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t build_number;
    uint32_t platform_id;
    minidump_rva_t csd_version_rva;
    uint16_t suite_mask;
    uint16_t reserved2;
    minidump_cpu_information_t cpu;
} PACKED_ALIGNED_ATTR(4) minidump_system_info_t;
PACKED_STRUCT_END

/**
 * Exception information
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t exception_code;
    uint32_t exception_flags;
    uint64_t exception_record;
    uint64_t exception_address;
    uint32_t number_parameters;
    uint32_t unused_alignment;
    uint64_t exception_information[15];
} PACKED_ATTR minidump_exception_record_t;
PACKED_STRUCT_END

/**
 * Exception stream
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t thread_id;
    uint32_t alignment;
    minidump_exception_record_t exception_record;
    minidump_location_t thread_context;
} PACKED_ATTR minidump_exception_stream_t;
PACKED_STRUCT_END

/**
 * Module (shared library) descriptor
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint64_t base_of_image;
    uint32_t size_of_image;
    uint32_t checksum;
    uint32_t time_date_stamp;
    minidump_rva_t module_name_rva;
    uint32_t
        version_info[13]; // VS_FIXEDFILEINFO: 13 uint32_t fields = 52 bytes
    minidump_location_t cv_record;
    minidump_location_t misc_record;
    uint64_t reserved0;
    uint64_t reserved1;
} PACKED_ATTR minidump_module_t;
PACKED_STRUCT_END

/**
 * Module list
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t count;
    minidump_module_t modules[]; // Variable length
} PACKED_ATTR minidump_module_list_t;
PACKED_STRUCT_END

/**
 * String (UTF-16LE for Windows compatibility)
 */
PACKED_STRUCT_BEGIN
typedef struct {
    uint32_t length; // In bytes, not including null terminator
    uint16_t buffer[]; // Variable length
} PACKED_ATTR minidump_string_t;
PACKED_STRUCT_END

#endif
