#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_MACOS)

#    include <errno.h>
#    include <fcntl.h>
#    include <mach-o/dyld.h>
#    include <mach-o/loader.h>
#    include <mach/mach.h>
#    include <mach/mach_vm.h>
#    include <pthread.h>
#    include <stdbool.h>
#    include <stdio.h>
#    include <string.h>
#    include <sys/stat.h>
#    include <sys/sysctl.h>
#    include <time.h>
#    include <unistd.h>

#    include "sentry_alloc.h"
#    include "sentry_logger.h"
#    include "sentry_minidump_format.h"
#    include "sentry_minidump_writer.h"

// Use shared constants from crash context
#    include "../sentry_crash_context.h"

// CodeView record format for storing UUID
// CV signature: 'RSDS' for PDB 7.0 format (we use it for Mach-O UUID too)
#    define CV_SIGNATURE_RSDS 0x53445352 // "RSDS" in little-endian

typedef struct {
    uint32_t cv_signature; // 'RSDS'
    uint8_t signature[16]; // UUID (matches Mach-O LC_UUID)
    uint32_t age; // Always 0 for Mach-O
    char pdb_file_name[1]; // Module path (variable length)
} __attribute__((packed)) cv_info_pdb70_t;

/**
 * Memory region info
 */
typedef struct {
    mach_vm_address_t address;
    mach_vm_size_t size;
    vm_prot_t protection;
} memory_region_t;

/**
 * Minidump writer context for macOS
 */
typedef struct {
    const sentry_crash_context_t *crash_ctx;
    int fd;
    uint32_t current_offset;

    task_t task;
    thread_array_t threads;
    mach_msg_type_number_t thread_count;

    memory_region_t regions[SENTRY_CRASH_MAX_MAPPINGS];
    size_t region_count;
} minidump_writer_t;

/**
 * Read memory from task
 */
static kern_return_t
read_task_memory(
    task_t task, mach_vm_address_t addr, void *buf, mach_vm_size_t size)
{
    mach_vm_size_t bytes_read = 0;
    return mach_vm_read_overwrite(
        task, addr, size, (mach_vm_address_t)buf, &bytes_read);
}

/**
 * Enumerate memory regions
 */
static int
enumerate_memory_regions(minidump_writer_t *writer)
{
    mach_vm_address_t address = 0;
    writer->region_count = 0;

    while (writer->region_count < SENTRY_CRASH_MAX_MAPPINGS) {
        mach_vm_size_t size = 0;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object_name = MACH_PORT_NULL;

        kern_return_t kr = mach_vm_region(writer->task, &address, &size,
            VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info, &info_count,
            &object_name);

        if (kr != KERN_SUCCESS) {
            break;
        }

        memory_region_t *region = &writer->regions[writer->region_count++];
        region->address = address;
        region->size = size;
        region->protection = info.protection;

        address += size;
    }

    SENTRY_DEBUGF("found %zu memory regions", writer->region_count);
    return 0;
}

/**
 * Write data to minidump file
 */
static minidump_rva_t
write_data(minidump_writer_t *writer, const void *data, size_t size)
{
    minidump_rva_t rva = writer->current_offset;

    ssize_t written = write(writer->fd, data, size);
    if (written != (ssize_t)size) {
        return 0;
    }

    writer->current_offset += size;

    // Align to 4-byte boundary
    uint32_t padding = (4 - (writer->current_offset % 4)) % 4;
    if (padding > 0) {
        const uint8_t zeros[4] = { 0 };
        write(writer->fd, zeros, padding);
        writer->current_offset += padding;
    }

    return rva;
}

/**
 * Write minidump header
 */
static int
write_header(minidump_writer_t *writer, uint32_t stream_count)
{
    minidump_header_t header = {
        .signature = MINIDUMP_SIGNATURE,
        .version = MINIDUMP_VERSION,
        .stream_count = stream_count,
        .stream_directory_rva = sizeof(minidump_header_t),
        .checksum = 0,
        .time_date_stamp = (uint32_t)time(NULL),
        .flags = 0,
    };

    return write_data(writer, &header, sizeof(header)) ? 0 : -1;
}

/**
 * Write a UTF-16 string to minidump (MINIDUMP_STRING format)
 * Returns RVA of the string
 */
static minidump_rva_t
write_minidump_string(minidump_writer_t *writer, const char *utf8_str)
{
    // Convert UTF-8 to UTF-16LE and write as MINIDUMP_STRING
    // Format: uint32_t length (in bytes, not including null terminator)
    //         followed by UTF-16LE characters with null terminator

    size_t utf8_len = utf8_str ? strlen(utf8_str) : 0;

    // For simplicity, assume ASCII (each char becomes 2 bytes in UTF-16)
    // Real implementation would need proper UTF-8 to UTF-16 conversion
    size_t utf16_len = utf8_len * 2; // Length in bytes

    uint32_t *buffer = sentry_malloc(
        sizeof(uint32_t) + utf16_len + 2); // +2 for null terminator
    if (!buffer) {
        return 0;
    }

    buffer[0]
        = (uint32_t)utf16_len; // Length in bytes (not including terminator)

    // Convert ASCII to UTF-16LE
    uint16_t *utf16_chars = (uint16_t *)&buffer[1];
    for (size_t i = 0; i < utf8_len; i++) {
        utf16_chars[i] = (uint16_t)(unsigned char)utf8_str[i];
    }
    utf16_chars[utf8_len] = 0; // Null terminator

    minidump_rva_t rva
        = write_data(writer, buffer, sizeof(uint32_t) + utf16_len + 2);
    sentry_free(buffer);

    return rva;
}

/**
 * Extract UUID from Mach-O file
 * Returns true if UUID found, false otherwise
 */
static bool
extract_macho_uuid(const char *macho_path, uint8_t uuid[16])
{
    int fd = open(macho_path, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    // Read Mach-O header
#    if defined(__x86_64__) || defined(__aarch64__)
    struct mach_header_64 header;
#    else
    struct mach_header header;
#    endif

    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return false;
    }

    // Verify Mach-O magic
#    if defined(__x86_64__)
    uint32_t expected_magic = MH_MAGIC_64;
#    elif defined(__aarch64__)
    uint32_t expected_magic = MH_MAGIC_64;
#    else
    uint32_t expected_magic = MH_MAGIC;
#    endif

    if (header.magic != expected_magic && header.magic != MH_CIGAM_64
        && header.magic != MH_CIGAM) {
        close(fd);
        return false;
    }

    // Read load commands
    size_t commands_size = header.sizeofcmds;
    void *commands_buf = sentry_malloc(commands_size);
    if (!commands_buf) {
        close(fd);
        return false;
    }

    if (read(fd, commands_buf, commands_size) != (ssize_t)commands_size) {
        sentry_free(commands_buf);
        close(fd);
        return false;
    }

    // Search for LC_UUID command
    uint8_t *ptr = (uint8_t *)commands_buf;
    bool found = false;
    for (uint32_t i = 0; i < header.ncmds && !found; i++) {
        struct load_command *cmd = (struct load_command *)ptr;

        if (cmd->cmd == LC_UUID) {
            struct uuid_command *uuid_cmd = (struct uuid_command *)ptr;
            memcpy(uuid, uuid_cmd->uuid, 16);
            found = true;
            break;
        }

        ptr += cmd->cmdsize;
    }

    sentry_free(commands_buf);
    close(fd);
    return found;
}

/**
 * Write CodeView record with UUID
 */
static minidump_rva_t
write_cv_record(
    minidump_writer_t *writer, const char *module_path, const uint8_t uuid[16])
{
    if (!uuid) {
        return 0;
    }

    // Calculate size: header + path + null terminator
    size_t path_len = strlen(module_path);
    size_t total_size
        = sizeof(cv_info_pdb70_t) + path_len; // +1 already in struct

    cv_info_pdb70_t *cv_record = sentry_malloc(total_size);
    if (!cv_record) {
        return 0;
    }

    cv_record->cv_signature = CV_SIGNATURE_RSDS;
    cv_record->age = 0; // Not used for Mach-O

    // Copy UUID
    memcpy(cv_record->signature, uuid, 16);

    // Copy module path
    memcpy(cv_record->pdb_file_name, module_path, path_len + 1);

    minidump_rva_t rva = write_data(writer, cv_record, total_size);
    sentry_free(cv_record);
    return rva;
}

/**
 * Write system info stream
 */
static int
write_system_info_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    minidump_system_info_t sysinfo = { 0 };

#    if defined(__x86_64__)
    sysinfo.processor_architecture = MINIDUMP_CPU_X86_64;
#    elif defined(__aarch64__) || defined(__arm64__)
    sysinfo.processor_architecture = MINIDUMP_CPU_ARM64;
#    elif defined(__i386__)
    sysinfo.processor_architecture = MINIDUMP_CPU_X86;
#    elif defined(__arm__)
    sysinfo.processor_architecture = MINIDUMP_CPU_ARM;
#    endif

    sysinfo.platform_id = MINIDUMP_OS_MACOS;

    int mib[2] = { CTL_HW, HW_NCPU };
    int ncpu = 1;
    size_t len = sizeof(ncpu);
    sysctl(mib, 2, &ncpu, &len, NULL, 0);
    sysinfo.number_of_processors = (uint8_t)ncpu;

    // Set processor level and revision (required for proper parsing)
#    if defined(__aarch64__) || defined(__arm64__)
    sysinfo.processor_level = 8; // ARM v8
    sysinfo.processor_revision = 0;
#    elif defined(__x86_64__)
    sysinfo.processor_level = 6; // P6 family
    sysinfo.processor_revision = 0;
#    endif

    // Set required OS and product type
    sysinfo.product_type = 1; // VER_NT_WORKSTATION

    // Get actual macOS version and build string
    char os_version[256];
    char build_version[256] = "";
    len = sizeof(os_version);
    if (sysctlbyname("kern.osproductversion", os_version, &len, NULL, 0) == 0) {
        // Parse version string like "14.0.0"
        int major = 0, minor = 0, patch = 0;
        sscanf(os_version, "%d.%d.%d", &major, &minor, &patch);
        sysinfo.major_version = major;
        sysinfo.minor_version = minor;
        sysinfo.build_number = patch;

        // Get build version for CSD string
        len = sizeof(build_version);
        sysctlbyname("kern.osversion", build_version, &len, NULL, 0);
    } else {
        // Fallback values
        sysinfo.major_version = 14;
        sysinfo.minor_version = 0;
        sysinfo.build_number = 0;
    }

    // Populate CPU information
#    if defined(__x86_64__) || defined(__i386__)
    // For x86/x86_64, we would populate vendor_id, version_information, etc.
    // For now, zero is acceptable
    memset(&sysinfo.cpu.x86_cpu_info, 0, sizeof(sysinfo.cpu.x86_cpu_info));
#    else
    // For ARM/ARM64 and other architectures, use processor_features
    // These are typically obtained from sysctl or cpuid-like mechanisms
    // For now, zero is acceptable (indicates no special features reported)
    memset(&sysinfo.cpu.other_cpu_info, 0, sizeof(sysinfo.cpu.other_cpu_info));
#    endif

    // Write CSD version string (required by Sentry)
    // Even if empty, must be present
    sysinfo.csd_version_rva
        = write_minidump_string(writer, build_version[0] ? build_version : "");
    if (!sysinfo.csd_version_rva) {
        return -1;
    }

    dir->stream_type = MINIDUMP_STREAM_SYSTEM_INFO;
    dir->rva = write_data(writer, &sysinfo, sizeof(sysinfo));
    dir->data_size = sizeof(sysinfo);

    return dir->rva ? 0 : -1;
}

/**
 * Get size of thread context for current architecture
 */
static size_t
get_context_size(void)
{
#    if defined(__x86_64__)
    return sizeof(minidump_context_x86_64_t);
#    elif defined(__aarch64__)
    return sizeof(minidump_context_arm64_t);
#    elif defined(__i386__)
    return sizeof(minidump_context_x86_t);
#    elif defined(__arm__)
    return sizeof(minidump_context_arm_t);
#    else
#        error "Unsupported architecture"
#    endif
}

/**
 * Convert macOS thread state to minidump context
 */
static minidump_rva_t
write_thread_context(
    minidump_writer_t *writer, const _STRUCT_MCONTEXT *mcontext)
{
#    if defined(__x86_64__)
    minidump_context_x86_64_t context = { 0 };
    // Set flags for full context (control + integer + segments + floating
    // point)
    context.context_flags
        = 0x0010003f; // CONTEXT_AMD64 | CONTEXT_CONTROL | CONTEXT_INTEGER |
                      // CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT

    // Copy general purpose registers
    context.rax = mcontext->__ss.__rax;
    context.rbx = mcontext->__ss.__rbx;
    context.rcx = mcontext->__ss.__rcx;
    context.rdx = mcontext->__ss.__rdx;
    context.rsi = mcontext->__ss.__rsi;
    context.rdi = mcontext->__ss.__rdi;
    context.rbp = mcontext->__ss.__rbp;
    context.rsp = mcontext->__ss.__rsp;
    context.r8 = mcontext->__ss.__r8;
    context.r9 = mcontext->__ss.__r9;
    context.r10 = mcontext->__ss.__r10;
    context.r11 = mcontext->__ss.__r11;
    context.r12 = mcontext->__ss.__r12;
    context.r13 = mcontext->__ss.__r13;
    context.r14 = mcontext->__ss.__r14;
    context.r15 = mcontext->__ss.__r15;
    context.rip = mcontext->__ss.__rip;
    context.eflags = mcontext->__ss.__rflags;
    context.cs = mcontext->__ss.__cs;
    context.fs = mcontext->__ss.__fs;
    context.gs = mcontext->__ss.__gs;

    // Copy FPU state from macOS float state
    context.mx_csr = mcontext->__fs.__fpu_mxcsr;
    context.float_save.control_word = mcontext->__fs.__fpu_fcw;
    context.float_save.status_word = mcontext->__fs.__fpu_fsw;
    context.float_save.tag_word = mcontext->__fs.__fpu_ftw;
    context.float_save.error_opcode = mcontext->__fs.__fpu_fop;
    context.float_save.error_offset = mcontext->__fs.__fpu_ip;
    context.float_save.data_offset = mcontext->__fs.__fpu_dp;
    context.float_save.mx_csr = mcontext->__fs.__fpu_mxcsr;
    context.float_save.mx_csr_mask = mcontext->__fs.__fpu_mxcsrmask;

    // Copy x87 FPU registers (ST0-ST7)
    for (int i = 0; i < 8; i++) {
        // macOS stores FPU registers as 10-byte values in __fpu_stmm0-7
        // We need to pack them into 128-bit values (only lower 80 bits are
        // valid)
        const uint8_t *fpreg
            = (const uint8_t *)&mcontext->__fs.__fpu_stmm0 + (i * 16);
        memcpy(&context.float_save.float_registers[i], fpreg, 16);
    }

    // Copy XMM registers (XMM0-XMM15)
    for (int i = 0; i < 16; i++) {
        const uint8_t *xmmreg
            = (const uint8_t *)&mcontext->__fs.__fpu_xmm0 + (i * 16);
        memcpy(&context.float_save.xmm_registers[i], xmmreg, 16);
    }

    return write_data(writer, &context, sizeof(context));

#    elif defined(__aarch64__)
    minidump_context_arm64_t context = { 0 };
    // Set flags for control + integer + fpsimd registers (FULL context)
    context.context_flags = 0x00400007; // ARM64 | Control | Integer | Fpsimd

    // Copy general purpose registers X0-X28
    for (int i = 0; i < 29; i++) {
        context.regs[i] = mcontext->__ss.__x[i];
    }
    // Copy FP, LR, SP, PC separately
    context.fp = mcontext->__ss.__fp; // X29
    context.lr = mcontext->__ss.__lr; // X30
    context.sp = mcontext->__ss.__sp;
    context.pc = mcontext->__ss.__pc;
    context.cpsr = mcontext->__ss.__cpsr;

    // Copy NEON/FP registers (V0-V31)
    memcpy(context.fpsimd, mcontext->__ns.__v, sizeof(mcontext->__ns.__v));
    context.fpsr = mcontext->__ns.__fpsr;
    context.fpcr = mcontext->__ns.__fpcr;

    // Zero out debug registers (not captured)
    memset(context.bcr, 0, sizeof(context.bcr));
    memset(context.bvr, 0, sizeof(context.bvr));
    memset(context.wcr, 0, sizeof(context.wcr));
    memset(context.wvr, 0, sizeof(context.wvr));

    return write_data(writer, &context, sizeof(context));

#    else
#        error "Unsupported architecture"
#    endif
}

/**
 * Read and write stack memory for a thread
 */
static minidump_rva_t
write_thread_stack(
    minidump_writer_t *writer, uint64_t stack_pointer, size_t *stack_size_out)
{
    // Read stack memory around SP
    // For safety, read a reasonable amount (64KB) from SP downwards
    const size_t MAX_STACK_SIZE = SENTRY_CRASH_MAX_STACK_CAPTURE / 8;

    // Stack grows downwards on macOS, so read from SP down to SP -
    // MAX_STACK_SIZE
    mach_vm_address_t stack_start = (stack_pointer > MAX_STACK_SIZE)
        ? (stack_pointer - MAX_STACK_SIZE)
        : 0;
    mach_vm_size_t stack_size = stack_pointer - stack_start;

    if (stack_size == 0 || stack_size > MAX_STACK_SIZE) {
        *stack_size_out = 0;
        return 0;
    }

    // Allocate buffer for stack memory
    void *stack_buffer = sentry_malloc(stack_size);
    if (!stack_buffer) {
        *stack_size_out = 0;
        return 0;
    }

    // Try to read stack memory
    kern_return_t kr
        = read_task_memory(writer->task, stack_start, stack_buffer, stack_size);

    minidump_rva_t rva = 0;
    if (kr == KERN_SUCCESS) {
        rva = write_data(writer, stack_buffer, stack_size);
        *stack_size_out = stack_size;
    } else {
        *stack_size_out = 0;
    }

    sentry_free(stack_buffer);
    return rva;
}

/**
 * Write thread list stream
 */
static int
write_thread_list_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    uint32_t thread_count = writer->thread_count;

    // In fallback mode (no task_for_pid), use threads from crash context
    if (thread_count == 0 && writer->crash_ctx) {
        if (writer->crash_ctx->platform.num_threads > 0) {
            thread_count = writer->crash_ctx->platform.num_threads;
            SENTRY_DEBUGF("Using %u threads from crash context", thread_count);
        } else {
            // Last resort: add at least the crashing thread
            thread_count = 1;
            SENTRY_WARN("No threads in crash context, using last resort path");
        }
    } else {
        SENTRY_DEBUGF("Using %u threads from task_threads()", thread_count);
    }

    size_t list_size
        = sizeof(uint32_t) + (thread_count * sizeof(minidump_thread_t));

    minidump_thread_list_t *thread_list = sentry_malloc(list_size);
    if (!thread_list) {
        return -1;
    }

    thread_list->count = thread_count;

    if (writer->thread_count > 0) {
        // Full path: enumerate all threads from task_threads()
        for (mach_msg_type_number_t i = 0; i < writer->thread_count; i++) {
            minidump_thread_t *thread = &thread_list->threads[i];
            memset(thread, 0, sizeof(*thread));

            thread_t mach_thread = writer->threads[i];

            // Get thread ID
            thread_identifier_info_data_t identifier_info;
            mach_msg_type_number_t identifier_info_count
                = THREAD_IDENTIFIER_INFO_COUNT;

            if (thread_info(mach_thread, THREAD_IDENTIFIER_INFO,
                    (thread_info_t)&identifier_info, &identifier_info_count)
                == KERN_SUCCESS) {
                thread->thread_id = identifier_info.thread_id;
            }

            // Get thread priority
            thread_extended_info_data_t extended_info;
            mach_msg_type_number_t extended_info_count
                = THREAD_EXTENDED_INFO_COUNT;

            if (thread_info(mach_thread, THREAD_EXTENDED_INFO,
                    (thread_info_t)&extended_info, &extended_info_count)
                == KERN_SUCCESS) {
                thread->priority = extended_info.pth_curpri;
                thread->priority_class = extended_info.pth_priority;
            }

            // Get thread state (registers)
            _STRUCT_MCONTEXT mcontext;
            mach_msg_type_number_t state_count = MACHINE_THREAD_STATE_COUNT;
            if (thread_get_state(mach_thread, MACHINE_THREAD_STATE,
                    (thread_state_t)&mcontext, &state_count)
                == KERN_SUCCESS) {

                // Write thread context (registers)
                thread->thread_context.rva
                    = write_thread_context(writer, &mcontext);
                thread->thread_context.size = get_context_size();

                // Write stack memory
                uint64_t sp;
#    if defined(__x86_64__)
                sp = mcontext.__ss.__rsp;
#    elif defined(__aarch64__)
                sp = mcontext.__ss.__sp;
#    endif
                size_t stack_size = 0;
                thread->stack.memory.rva
                    = write_thread_stack(writer, sp, &stack_size);
                thread->stack.memory.size = stack_size;
                thread->stack.start_address = sp;
            }
        }
    } else if (writer->crash_ctx
        && writer->crash_ctx->platform.num_threads > 0) {
        // Fallback path: use threads captured in signal handler
        for (size_t i = 0;
            i < writer->crash_ctx->platform.num_threads && i < thread_count;
            i++) {
            minidump_thread_t *thread = &thread_list->threads[i];
            memset(thread, 0, sizeof(*thread));

            // Use thread ID captured in signal handler (portable across
            // processes)
            thread->thread_id = writer->crash_ctx->platform.threads[i].tid;

            // Write thread context (registers)
            const _STRUCT_MCONTEXT *state
                = &writer->crash_ctx->platform.threads[i].state;
            thread->thread_context.rva = write_thread_context(writer, state);
            thread->thread_context.size = get_context_size();
            SENTRY_DEBUGF("Thread %zu: wrote context at RVA 0x%x", i,
                thread->thread_context.rva);

            // Write stack memory from file (captured in signal handler)
            uint64_t sp;
#    if defined(__x86_64__)
            sp = state->__ss.__rsp;
#    elif defined(__aarch64__)
            sp = state->__ss.__sp;
#    endif

            const char *stack_path
                = writer->crash_ctx->platform.threads[i].stack_path;
            uint64_t saved_stack_size
                = writer->crash_ctx->platform.threads[i].stack_size;

            if (stack_path[0] != '\0' && saved_stack_size > 0) {
                // Read stack from file
                int stack_fd = open(stack_path, O_RDONLY);
                if (stack_fd >= 0) {
                    void *stack_buffer = sentry_malloc(saved_stack_size);
                    if (stack_buffer) {
                        ssize_t bytes_read
                            = read(stack_fd, stack_buffer, saved_stack_size);
                        if (bytes_read == (ssize_t)saved_stack_size) {
                            thread->stack.memory.rva = write_data(
                                writer, stack_buffer, saved_stack_size);
                            thread->stack.memory.size = saved_stack_size;
                            // Stack memory starts at SP (we captured from SP
                            // upwards)
                            thread->stack.start_address = sp;
                            SENTRY_DEBUGF(
                                "Thread %zu: wrote stack from file at RVA "
                                "0x%x, size %llu, start_addr 0x%llx",
                                i, thread->stack.memory.rva,
                                (unsigned long long)saved_stack_size,
                                (unsigned long long)sp);
                        } else {
                            SENTRY_WARN("Failed to read stack file");
                            thread->stack.memory.rva = 0;
                            thread->stack.memory.size = 0;
                        }
                        sentry_free(stack_buffer);
                    }
                    close(stack_fd);
                    // Delete stack file after reading
                    unlink(stack_path);
                } else {
                    SENTRY_WARNF("Failed to open stack file: %s", stack_path);
                    thread->stack.memory.rva = 0;
                    thread->stack.memory.size = 0;
                }
            } else {
                // No saved stack, try to read from memory (will likely fail
                // without task port)
                size_t stack_size = 0;
                thread->stack.memory.rva
                    = write_thread_stack(writer, sp, &stack_size);
                thread->stack.memory.size = stack_size;
                thread->stack.start_address = sp;
                SENTRY_DEBUGF(
                    "Thread %zu: wrote stack from memory at RVA 0x%x, size %zu",
                    i, thread->stack.memory.rva, stack_size);
            }
        }
    } else if (writer->crash_ctx) {
        // Last resort: add just the crashing thread ID
        minidump_thread_t *thread = &thread_list->threads[0];
        memset(thread, 0, sizeof(*thread));
        thread->thread_id = writer->crash_ctx->crashed_tid;
    }

    dir->stream_type = MINIDUMP_STREAM_THREAD_LIST;
    dir->rva = write_data(writer, thread_list, list_size);
    dir->data_size = list_size;

    sentry_free(thread_list);
    return dir->rva ? 0 : -1;
}

/**
 * Write exception stream
 */
static int
write_exception_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    minidump_exception_stream_t exception_stream = { 0 };

    exception_stream.thread_id = writer->crash_ctx->crashed_tid;
    exception_stream.exception_record.exception_code
        = 0x40000000 | writer->crash_ctx->platform.signum;
    exception_stream.exception_record.exception_flags = 0;
    exception_stream.exception_record.exception_address
        = (uint64_t)writer->crash_ctx->platform.siginfo.si_addr;
    exception_stream.exception_record.number_parameters = 0;

    // Write the crashing thread's context
    // Use the context from the first thread in the crash context (the crashing
    // thread)
    if (writer->crash_ctx->platform.num_threads > 0) {
        const _STRUCT_MCONTEXT *crash_state
            = &writer->crash_ctx->platform.threads[0].state;
        exception_stream.thread_context.rva
            = write_thread_context(writer, crash_state);
        exception_stream.thread_context.size = get_context_size();
        SENTRY_DEBUGF("Exception: wrote context at RVA 0x%x for thread %u",
            exception_stream.thread_context.rva, exception_stream.thread_id);
    }

    dir->stream_type = MINIDUMP_STREAM_EXCEPTION;
    dir->rva = write_data(writer, &exception_stream, sizeof(exception_stream));
    dir->data_size = sizeof(exception_stream);

    return dir->rva ? 0 : -1;
}

/**
 * Write module list stream (using pre-captured modules from crash context)
 */
static int
write_module_list_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    // Use modules from crash context (captured in signal handler)
    uint32_t module_count = writer->crash_ctx->module_count;

    size_t list_size
        = sizeof(uint32_t) + (module_count * sizeof(minidump_module_t));
    minidump_module_list_t *module_list = sentry_malloc(list_size);
    if (!module_list) {
        return -1;
    }

    module_list->count = module_count;

    for (uint32_t i = 0; i < module_count; i++) {
        minidump_module_t *mdmodule = &module_list->modules[i];
        memset(mdmodule, 0, sizeof(*mdmodule));

        const sentry_module_info_t *module = &writer->crash_ctx->modules[i];

        // Set module base address and size
        mdmodule->base_of_image = module->base_address;
        mdmodule->size_of_image = module->size;

        // Set VS_FIXEDFILEINFO signature (first uint32_t of version_info)
        // This is required for minidump processors to recognize the module
        uint32_t version_sig = 0xFEEF04BD;
        memcpy(&mdmodule->version_info[0], &version_sig, sizeof(version_sig));

        // Write module name as UTF-16 string
        mdmodule->module_name_rva = write_minidump_string(writer, module->name);

        // Write CodeView record with UUID for symbolication
        // Try to use UUID captured in signal handler first
        uint8_t uuid[16];
        bool has_uuid = false;

        // Check if UUID was captured in signal handler
        bool uuid_is_zero = true;
        for (int j = 0; j < 16; j++) {
            if (module->uuid[j] != 0) {
                uuid_is_zero = false;
                break;
            }
        }

        if (!uuid_is_zero) {
            // Use UUID from signal handler
            memcpy(uuid, module->uuid, 16);
            has_uuid = true;
        } else {
            // Fallback: Extract UUID from Mach-O file
            has_uuid = extract_macho_uuid(module->name, uuid);
        }

        if (has_uuid) {
            minidump_rva_t cv_rva = write_cv_record(writer, module->name, uuid);
            if (cv_rva) {
                mdmodule->cv_record.rva = cv_rva;
                mdmodule->cv_record.size
                    = sizeof(cv_info_pdb70_t) + strlen(module->name);
            }
        }
    }

    dir->stream_type = MINIDUMP_STREAM_MODULE_LIST;
    dir->rva = write_data(writer, module_list, list_size);
    dir->data_size = list_size;

    sentry_free(module_list);
    return dir->rva ? 0 : -1;
}

/**
 * Write misc info stream (MINIDUMP_MISC_INFO)
 */
static int
write_misc_info_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    // MINIDUMP_MISC_INFO structure
    struct {
        uint32_t size_of_info;
        uint32_t flags1;
        uint32_t process_id;
        uint32_t process_create_time;
        uint32_t process_user_time;
        uint32_t process_kernel_time;
    } __attribute__((packed, aligned(4))) misc_info = { 0 };

    misc_info.size_of_info = sizeof(misc_info);
    misc_info.flags1 = 0x00000001; // MINIDUMP_MISC1_PROCESS_ID
    misc_info.process_id = writer->crash_ctx->crashed_pid;
    misc_info.process_create_time = 0;
    misc_info.process_user_time = 0;
    misc_info.process_kernel_time = 0;

    dir->stream_type = 15; // MiscInfoStream
    dir->rva = write_data(writer, &misc_info, sizeof(misc_info));
    dir->data_size = sizeof(misc_info);

    return dir->rva ? 0 : -1;
}

/**
 * Check if a memory region should be included based on minidump mode
 */
static bool
should_include_region_macos(
    const memory_region_t *region, sentry_minidump_mode_t mode)
{
    // STACK_ONLY: Don't include heap regions (stack is in thread list)
    if (mode == SENTRY_MINIDUMP_MODE_STACK_ONLY) {
        return false;
    }

    // FULL: Include all readable regions
    if (mode == SENTRY_MINIDUMP_MODE_FULL) {
        return (region->protection & VM_PROT_READ) != 0;
    }

    // SMART: Include writable regions (heap), exclude read-only (code/data)
    if (mode == SENTRY_MINIDUMP_MODE_SMART) {
        // Include regions that are readable and writable (heap allocations)
        bool readable = (region->protection & VM_PROT_READ) != 0;
        bool writable = (region->protection & VM_PROT_WRITE) != 0;

        if (readable && writable) {
            // Limit to reasonable size (64MB per region)
            return region->size <= (SENTRY_CRASH_MAX_STACK_CAPTURE / 8 * 1024);
        }
    }

    return false;
}

/**
 * Write memory list stream with memory based on minidump mode
 */
static int
write_memory_list_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    sentry_minidump_mode_t mode = writer->crash_ctx->minidump_mode;

    // STACK_ONLY: Don't write memory list (stack is in thread list already)
    if (mode == SENTRY_MINIDUMP_MODE_STACK_ONLY) {
        uint32_t count = 0;
        dir->stream_type = MINIDUMP_STREAM_MEMORY_LIST;
        dir->rva = write_data(writer, &count, sizeof(count));
        dir->data_size = sizeof(count);
        return dir->rva ? 0 : -1;
    }

    // For SMART and FULL modes, capture memory regions
    // Count regions to include
    size_t region_count = 0;
    for (size_t i = 0; i < writer->region_count; i++) {
        if (should_include_region_macos(&writer->regions[i], mode)) {
            region_count++;
        }
    }

    // Allocate memory list
    size_t list_size = sizeof(uint32_t)
        + (region_count * sizeof(minidump_memory_descriptor_t));
    minidump_memory_list_t *memory_list = sentry_malloc(list_size);
    if (!memory_list) {
        // Fallback to empty list
        uint32_t count = 0;
        dir->stream_type = MINIDUMP_STREAM_MEMORY_LIST;
        dir->rva = write_data(writer, &count, sizeof(count));
        dir->data_size = sizeof(count);
        return dir->rva ? 0 : -1;
    }

    memory_list->count = region_count;

    // Write memory regions
    size_t mem_idx = 0;
    for (size_t i = 0; i < writer->region_count && mem_idx < region_count;
        i++) {
        if (!should_include_region_macos(&writer->regions[i], mode)) {
            continue;
        }

        memory_region_t *region = &writer->regions[i];
        minidump_memory_descriptor_t *mem = &memory_list->ranges[mem_idx++];

        mach_vm_size_t region_size = region->size;

        // Limit individual region size
        const size_t MAX_REGION_SIZE
            = SENTRY_CRASH_MAX_STACK_CAPTURE / 8 * 1024; // 64MB
        if (region_size > MAX_REGION_SIZE) {
            region_size = MAX_REGION_SIZE;
        }

        // Allocate buffer for region memory
        void *region_buffer = sentry_malloc(region_size);
        if (!region_buffer) {
            mem->start_address = region->address;
            mem->memory.size = 0;
            mem->memory.rva = 0;
            continue;
        }

        // Try to read memory from task
        kern_return_t kr = read_task_memory(
            writer->task, region->address, region_buffer, region_size);

        if (kr == KERN_SUCCESS) {
            mem->start_address = region->address;
            mem->memory.rva = write_data(writer, region_buffer, region_size);
            mem->memory.size = region_size;
        } else {
            mem->start_address = region->address;
            mem->memory.size = 0;
            mem->memory.rva = 0;
        }

        sentry_free(region_buffer);
    }

    dir->stream_type = MINIDUMP_STREAM_MEMORY_LIST;
    dir->rva = write_data(writer, memory_list, list_size);
    dir->data_size = list_size;

    sentry_free(memory_list);
    return dir->rva ? 0 : -1;
}

/**
 * Main minidump writer for macOS
 */
int
sentry__write_minidump(
    const sentry_crash_context_t *ctx, const char *output_path)
{
    // For now, write a minimal but valid minidump with just the crash context
    // Full memory dump would require task_for_pid entitlements

    SENTRY_DEBUG("write_minidump: starting");

    minidump_writer_t writer = { 0 };
    writer.crash_ctx = ctx;

    // Open output file
    SENTRY_DEBUGF("write_minidump: opening file %s", output_path);
    writer.fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (writer.fd < 0) {
        SENTRY_WARN("write_minidump: failed to open file");
        return -1;
    }
    SENTRY_DEBUGF("write_minidump: file opened, fd=%d", writer.fd);

    // Try to get task port for crashed process (may fail without entitlements)
    SENTRY_DEBUG("write_minidump: getting task port");
    kern_return_t kr
        = task_for_pid(mach_task_self(), ctx->crashed_pid, &writer.task);
    if (kr != KERN_SUCCESS) {
        SENTRY_DEBUGF("write_minidump: task_for_pid failed (%d), writing "
                      "minimal minidump",
            kr);
        // Without task port, write minimal minidump with all required streams
        // Matching Crashpad's minimum: SystemInfo, MiscInfo, ThreadList,
        // Exception, ModuleList, MemoryList
        writer.task = MACH_PORT_NULL;
        writer.thread_count = 0;

        // Reserve space for header and directory (6 streams), position file
        // after them
        const uint32_t stream_count = 6;
        writer.current_offset = sizeof(minidump_header_t)
            + stream_count * sizeof(minidump_directory_t);
        SENTRY_DEBUG("write_minidump: seeking to stream offset");
        if (lseek(writer.fd, writer.current_offset, SEEK_SET) < 0) {
            SENTRY_WARN("write_minidump: lseek failed");
            close(writer.fd);
            return -1;
        }

        // Write streams in same order as Crashpad (will update directory RVAs
        // and current_offset)
        minidump_directory_t directories[6] = { 0 };
        SENTRY_DEBUG("write_minidump: writing system_info stream");
        if (write_system_info_stream(&writer, &directories[0]) < 0) {
            SENTRY_WARN("write_minidump: system_info failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: writing misc_info stream");
        if (write_misc_info_stream(&writer, &directories[1]) < 0) {
            SENTRY_WARN("write_minidump: misc_info failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: writing thread_list stream");
        if (write_thread_list_stream(&writer, &directories[2]) < 0) {
            SENTRY_WARN("write_minidump: thread_list failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: writing exception stream");
        if (write_exception_stream(&writer, &directories[3]) < 0) {
            SENTRY_WARN("write_minidump: exception failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: writing module_list stream");
        if (write_module_list_stream(&writer, &directories[4]) < 0) {
            SENTRY_WARN("write_minidump: module_list failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: writing memory_list stream");
        if (write_memory_list_stream(&writer, &directories[5]) < 0) {
            SENTRY_WARN("write_minidump: memory_list failed");
            close(writer.fd);
            return -1;
        }
        SENTRY_DEBUG("write_minidump: all streams written");

        // Now write header and directory at the beginning
        SENTRY_DEBUG("write_minidump: seeking to beginning for header");
        if (lseek(writer.fd, 0, SEEK_SET) < 0) {
            SENTRY_WARN("write_minidump: lseek to beginning failed");
            close(writer.fd);
            return -1;
        }

        SENTRY_DEBUG("write_minidump: writing header");
        minidump_header_t header = { .signature = MINIDUMP_SIGNATURE,
            .version = MINIDUMP_VERSION,
            .stream_count = stream_count,
            .stream_directory_rva = sizeof(minidump_header_t),
            .checksum = 0,
            .time_date_stamp = (uint32_t)time(NULL),
            .flags = 0 };
        if (write(writer.fd, &header, sizeof(header)) != sizeof(header)) {
            SENTRY_WARN("write_minidump: header write failed");
            close(writer.fd);
            return -1;
        }

        SENTRY_DEBUG("write_minidump: writing directory");
        // Write directory
        if (write(writer.fd, directories, sizeof(directories))
            != sizeof(directories)) {
            SENTRY_WARN("write_minidump: directory write failed");
            close(writer.fd);
            return -1;
        }

        SENTRY_DEBUG("write_minidump: closing file");
        close(writer.fd);
        SENTRY_DEBUG("write_minidump: success");
        return 0;
    }

    // Get threads
    kr = task_threads(writer.task, &writer.threads, &writer.thread_count);
    if (kr != KERN_SUCCESS) {
        SENTRY_WARNF("failed to get threads: %d", kr);
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Enumerate memory regions
    enumerate_memory_regions(&writer);

    // Reserve space for header and directory
    const uint32_t stream_count = 3; // system_info, threads, exception
    writer.current_offset = sizeof(minidump_header_t)
        + (stream_count * sizeof(minidump_directory_t));

    if (lseek(writer.fd, writer.current_offset, SEEK_SET) < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Write streams
    minidump_directory_t directories[3];
    int result = 0;

    result |= write_system_info_stream(&writer, &directories[0]);
    result |= write_thread_list_stream(&writer, &directories[1]);
    result |= write_exception_stream(&writer, &directories[2]);

    if (result < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Write header and directory
    if (lseek(writer.fd, 0, SEEK_SET) < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    if (write_header(&writer, stream_count) < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    if (write(writer.fd, directories, sizeof(directories))
        != sizeof(directories)) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Cleanup
    for (mach_msg_type_number_t i = 0; i < writer.thread_count; i++) {
        mach_port_deallocate(mach_task_self(), writer.threads[i]);
    }
    vm_deallocate(mach_task_self(), (vm_address_t)writer.threads,
        writer.thread_count * sizeof(thread_t));

    close(writer.fd);

    SENTRY_DEBUG("successfully wrote minidump");
    return 0;
}

#endif // SENTRY_PLATFORM_MACOS
