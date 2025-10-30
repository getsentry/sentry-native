#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <dirent.h>
#    include <elf.h>
#    include <errno.h>
#    include <fcntl.h>
#    include <stdbool.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/ptrace.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <sys/uio.h>
#    include <sys/user.h>
#    include <sys/wait.h>
#    include <time.h>
#    include <unistd.h>

#    include "sentry_alloc.h"
#    include "sentry_logger.h"
#    include "sentry_minidump_format.h"
#    include "sentry_minidump_writer.h"

// NT_PRSTATUS is defined in linux/elf.h but we can't include that
// because it conflicts with elf.h. Define it here if not available.
#    ifndef NT_PRSTATUS
#        define NT_PRSTATUS 1
#    endif

#    if defined(__x86_64__)
// x86_64 FPU state structure from Linux kernel (matches _fpstate)
// This is what uc_mcontext.fpregs points to on Linux x86_64
struct linux_fxsave {
    uint16_t cwd; // Control word
    uint16_t swd; // Status word
    uint16_t ftw; // Tag word
    uint16_t fop; // Last instruction opcode
    uint64_t rip; // Instruction pointer
    uint64_t rdp; // Data pointer
    uint32_t mxcsr; // MXCSR register
    uint32_t mxcsr_mask; // MXCSR mask
    uint32_t st_space[32]; // ST0-ST7 (8 registers, 16 bytes each = 128 bytes)
    uint32_t
        xmm_space[64]; // XMM0-XMM15 (16 registers, 16 bytes each = 256 bytes)
    uint32_t padding[24];
};
#    endif

// CodeView record format for ELF modules with Build ID
// CV signature: 'BpEL' (Breakpad ELF) - compatible with Breakpad/Crashpad
#    define CV_SIGNATURE_ELF 0x4270454c // "BpEL" in little-endian

typedef struct {
    uint32_t cv_signature; // 'BpEL' (0x4270454c)
    uint8_t build_id[1]; // Variable length Build ID from ELF .note.gnu.build-id
                         // Typically 20 bytes (SHA-1) but can vary
} __attribute__((packed)) cv_info_elf_t;

#    if defined(__aarch64__)
// ARM64 signal context structures for accessing FPSIMD state
#        define FPSIMD_MAGIC 0x46508001

// Only define these if not already provided by system headers
#        ifndef __ASM_SIGCONTEXT_H
// Base header for context blocks in __reserved
struct _aarch64_ctx {
    uint32_t magic;
    uint32_t size;
};

// FPSIMD context containing NEON/FP registers
struct fpsimd_context {
    struct _aarch64_ctx head;
    uint32_t fpsr;
    uint32_t fpcr;
    __uint128_t vregs[32];
};
#        endif
#    endif

// Use process_vm_readv to read memory from crashed process
#    include <sys/uio.h>

// Use shared constants from crash context
#    include "../sentry_crash_context.h"

/**
 * Memory mapping from /proc/[pid]/maps
 */
typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t offset;
    char permissions[5]; // "rwxp"
    char name[256];
} memory_mapping_t;

/**
 * Minidump writer context
 */
typedef struct {
    const sentry_crash_context_t *crash_ctx;
    int fd;
    uint32_t current_offset;

    // Memory mappings
    memory_mapping_t mappings[SENTRY_CRASH_MAX_MAPPINGS];
    size_t mapping_count;

    // Threads
    pid_t tids[SENTRY_CRASH_MAX_THREADS];
    size_t thread_count;

    // Ptrace state
    bool ptrace_attached;
} minidump_writer_t;

/**
 * Attach to process using ptrace (must be called once before reading memory)
 */
static bool
ptrace_attach_process(minidump_writer_t *writer)
{
    if (writer->ptrace_attached) {
        return true;
    }

    pid_t pid = writer->crash_ctx->crashed_pid;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) != 0) {
        SENTRY_WARNF("ptrace(PTRACE_ATTACH) failed for PID %d: %s", pid,
            strerror(errno));
        return false;
    }

    // Wait for process to stop
    int status;
    if (waitpid(pid, &status, __WALL) < 0) {
        SENTRY_WARNF("waitpid after PTRACE_ATTACH failed for PID %d: %s", pid,
            strerror(errno));
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return false;
    }

    writer->ptrace_attached = true;
    SENTRY_DEBUGF("Successfully attached to process %d via ptrace", pid);
    return true;
}

/**
 * Get thread registers via ptrace (for non-crashed threads)
 * Returns true if registers were successfully captured
 */
static bool
ptrace_get_thread_registers(pid_t tid, ucontext_t *uctx)
{
    // Attach to the specific thread
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) != 0) {
        SENTRY_DEBUGF("ptrace(PTRACE_ATTACH) failed for thread %d: %s", tid,
            strerror(errno));
        return false;
    }

    // Wait for thread to stop
    int status;
    if (waitpid(tid, &status, __WALL) < 0) {
        SENTRY_DEBUGF("waitpid after PTRACE_ATTACH failed for thread %d: %s",
            tid, strerror(errno));
        ptrace(PTRACE_DETACH, tid, NULL, NULL);
        return false;
    }

    // Get general purpose registers
    bool success = false;

#    if defined(__x86_64__)
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, tid, NULL, &regs) == 0) {
        // Map to ucontext_t format
        uctx->uc_mcontext.gregs[REG_R8] = regs.r8;
        uctx->uc_mcontext.gregs[REG_R9] = regs.r9;
        uctx->uc_mcontext.gregs[REG_R10] = regs.r10;
        uctx->uc_mcontext.gregs[REG_R11] = regs.r11;
        uctx->uc_mcontext.gregs[REG_R12] = regs.r12;
        uctx->uc_mcontext.gregs[REG_R13] = regs.r13;
        uctx->uc_mcontext.gregs[REG_R14] = regs.r14;
        uctx->uc_mcontext.gregs[REG_R15] = regs.r15;
        uctx->uc_mcontext.gregs[REG_RDI] = regs.rdi;
        uctx->uc_mcontext.gregs[REG_RSI] = regs.rsi;
        uctx->uc_mcontext.gregs[REG_RBP] = regs.rbp;
        uctx->uc_mcontext.gregs[REG_RBX] = regs.rbx;
        uctx->uc_mcontext.gregs[REG_RDX] = regs.rdx;
        uctx->uc_mcontext.gregs[REG_RAX] = regs.rax;
        uctx->uc_mcontext.gregs[REG_RCX] = regs.rcx;
        uctx->uc_mcontext.gregs[REG_RSP] = regs.rsp;
        uctx->uc_mcontext.gregs[REG_RIP] = regs.rip;
        uctx->uc_mcontext.gregs[REG_EFL] = regs.eflags;
        uctx->uc_mcontext.gregs[REG_CSGSFS]
            = (regs.cs & 0xffff) | ((regs.gs & 0xffff) << 16);
        uctx->uc_mcontext.gregs[REG_ERR] = 0;
        uctx->uc_mcontext.gregs[REG_TRAPNO] = 0;
        uctx->uc_mcontext.gregs[REG_OLDMASK] = 0;
        uctx->uc_mcontext.gregs[REG_CR2] = 0;
        success = true;
        SENTRY_DEBUGF("Thread %d: captured registers via ptrace, SP=0x%llx",
            tid, (unsigned long long)regs.rsp);
    } else {
        SENTRY_DEBUGF("ptrace(PTRACE_GETREGS) failed for thread %d: %s", tid,
            strerror(errno));
    }
#    elif defined(__aarch64__)
    struct user_regs_struct regs;
    struct iovec iov;
    iov.iov_base = &regs;
    iov.iov_len = sizeof(regs);
    if (ptrace(PTRACE_GETREGSET, tid, (void *)NT_PRSTATUS, &iov) == 0) {
        // Map to ucontext_t format
        for (int i = 0; i < 31; i++) {
            uctx->uc_mcontext.regs[i] = regs.regs[i];
        }
        uctx->uc_mcontext.sp = regs.sp;
        uctx->uc_mcontext.pc = regs.pc;
        uctx->uc_mcontext.pstate = regs.pstate;
        success = true;
        SENTRY_DEBUGF("Thread %d: captured registers via ptrace, SP=0x%llx",
            tid, (unsigned long long)regs.sp);
    } else {
        SENTRY_DEBUGF("ptrace(PTRACE_GETREGSET) failed for thread %d: %s", tid,
            strerror(errno));
    }
#    elif defined(__i386__)
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, tid, NULL, &regs) == 0) {
        // Map to ucontext_t format
        uctx->uc_mcontext.gregs[REG_GS] = regs.xgs;
        uctx->uc_mcontext.gregs[REG_FS] = regs.xfs;
        uctx->uc_mcontext.gregs[REG_ES] = regs.xes;
        uctx->uc_mcontext.gregs[REG_DS] = regs.xds;
        uctx->uc_mcontext.gregs[REG_EDI] = regs.edi;
        uctx->uc_mcontext.gregs[REG_ESI] = regs.esi;
        uctx->uc_mcontext.gregs[REG_EBP] = regs.ebp;
        uctx->uc_mcontext.gregs[REG_ESP] = regs.esp;
        uctx->uc_mcontext.gregs[REG_EBX] = regs.ebx;
        uctx->uc_mcontext.gregs[REG_EDX] = regs.edx;
        uctx->uc_mcontext.gregs[REG_ECX] = regs.ecx;
        uctx->uc_mcontext.gregs[REG_EAX] = regs.eax;
        uctx->uc_mcontext.gregs[REG_TRAPNO] = 0;
        uctx->uc_mcontext.gregs[REG_ERR] = 0;
        uctx->uc_mcontext.gregs[REG_EIP] = regs.eip;
        uctx->uc_mcontext.gregs[REG_CS] = regs.xcs;
        uctx->uc_mcontext.gregs[REG_EFL] = regs.eflags;
        uctx->uc_mcontext.gregs[REG_UESP] = regs.esp;
        uctx->uc_mcontext.gregs[REG_SS] = regs.xss;
        success = true;
        SENTRY_DEBUGF(
            "Thread %d: captured registers via ptrace, SP=0x%x", tid, regs.esp);
    } else {
        SENTRY_DEBUGF("ptrace(PTRACE_GETREGS) failed for thread %d: %s", tid,
            strerror(errno));
    }
#    endif

    // Detach from thread
    ptrace(PTRACE_DETACH, tid, NULL, NULL);
    return success;
}

/**
 * Read memory from crashed process using ptrace
 */
static ssize_t
read_process_memory(
    minidump_writer_t *writer, uint64_t addr, void *buf, size_t len)
{
    if (!ptrace_attach_process(writer)) {
        return -1;
    }

    pid_t pid = writer->crash_ctx->crashed_pid;

    // Read memory word-by-word using ptrace(PTRACE_PEEKDATA)
    size_t bytes_read = 0;
    uint8_t *byte_buf = (uint8_t *)buf;
    uint64_t current_addr = addr;

    while (bytes_read < len) {
        // Align to word boundary for ptrace
        uint64_t aligned_addr = current_addr & ~(sizeof(long) - 1);
        size_t offset_in_word = current_addr - aligned_addr;

        errno = 0;
        long word = ptrace(PTRACE_PEEKDATA, pid, aligned_addr, NULL);
        if (errno != 0) {
            if (bytes_read > 0) {
                // Return partial read
                return bytes_read;
            }
            SENTRY_DEBUGF("ptrace(PTRACE_PEEKDATA) failed at 0x%llx: %s",
                (unsigned long long)aligned_addr, strerror(errno));
            return -1;
        }

        // Copy relevant bytes from this word
        uint8_t *word_bytes = (uint8_t *)&word;
        size_t bytes_from_word
            = sizeof(long) - offset_in_word < len - bytes_read
            ? sizeof(long) - offset_in_word
            : len - bytes_read;

        memcpy(byte_buf + bytes_read, word_bytes + offset_in_word,
            bytes_from_word);

        bytes_read += bytes_from_word;
        current_addr += bytes_from_word;
    }

    return bytes_read;
}

/**
 * Parse /proc/[pid]/maps to get memory mappings
 */
static int
parse_proc_maps(minidump_writer_t *writer)
{
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps",
        writer->crash_ctx->crashed_pid);

    FILE *f = fopen(maps_path, "r");
    if (!f) {
        SENTRY_WARNF("failed to open %s: %s", maps_path, strerror(errno));
        return -1;
    }

    char line[1024];
    writer->mapping_count = 0;

    while (fgets(line, sizeof(line), f)
        && writer->mapping_count < SENTRY_CRASH_MAX_MAPPINGS) {
        memory_mapping_t *mapping = &writer->mappings[writer->mapping_count];

        // Parse line: "start-end perms offset dev inode pathname"
        unsigned long long start, end, offset;
        char perms[5];
        int pathname_offset = 0;

        int parsed = sscanf(line, "%llx-%llx %4s %llx %*s %*s %n", &start, &end,
            perms, &offset, &pathname_offset);

        if (parsed >= 4) {
            mapping->start = start;
            mapping->end = end;
            mapping->offset = offset;
            memcpy(mapping->permissions, perms, 4);
            mapping->permissions[4] = '\0';

            // Extract pathname if present
            if (pathname_offset > 0 && line[pathname_offset] != '\0') {
                const char *pathname = line + pathname_offset;
                // Trim newline
                size_t len = strlen(pathname);
                if (len > 0 && pathname[len - 1] == '\n') {
                    len--;
                }
                size_t copy_len = len < sizeof(mapping->name) - 1
                    ? len
                    : sizeof(mapping->name) - 1;
                memcpy(mapping->name, pathname, copy_len);
                mapping->name[copy_len] = '\0';
            } else {
                mapping->name[0] = '\0';
            }

            writer->mapping_count++;
        }
    }

    fclose(f);

    SENTRY_DEBUGF("parsed %zu memory mappings", writer->mapping_count);
    return 0;
}

/**
 * Enumerate threads from /proc/[pid]/task
 */
static int
enumerate_threads(minidump_writer_t *writer)
{
    char task_path[64];
    snprintf(task_path, sizeof(task_path), "/proc/%d/task",
        writer->crash_ctx->crashed_pid);

    DIR *dir = opendir(task_path);
    if (!dir) {
        SENTRY_WARNF("failed to open %s: %s", task_path, strerror(errno));
        return -1;
    }

    writer->thread_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir))
        && writer->thread_count < SENTRY_CRASH_MAX_THREADS) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        pid_t tid = (pid_t)atoi(entry->d_name);
        if (tid > 0) {
            writer->tids[writer->thread_count++] = tid;
        }
    }

    closedir(dir);

    SENTRY_DEBUGF("found %zu threads", writer->thread_count);
    return 0;
}

/**
 * Write data to minidump file and return RVA
 */
static minidump_rva_t
write_data(minidump_writer_t *writer, const void *data, size_t size)
{
    minidump_rva_t rva = writer->current_offset;

    ssize_t written = write(writer->fd, data, size);
    if (written != (ssize_t)size) {
        SENTRY_WARNF("write failed: %s", strerror(errno));
        return 0;
    }

    writer->current_offset += size;

    // Align to 4-byte boundary
    uint32_t padding = (4 - (writer->current_offset % 4)) % 4;
    if (padding > 0) {
        const uint8_t zeros[4] = { 0 };
        if (write(writer->fd, zeros, padding) != (ssize_t)padding) {
            SENTRY_WARN("Failed to write padding bytes");
        }
        writer->current_offset += padding;
    }

    return rva;
}

/**
 * Write minidump header and directory
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

    if (write_data(writer, &header, sizeof(header)) == 0) {
        return -1;
    }

    return 0;
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
#    elif defined(__aarch64__)
    sysinfo.processor_architecture = MINIDUMP_CPU_ARM64;
#    elif defined(__i386__)
    sysinfo.processor_architecture = MINIDUMP_CPU_X86;
#    elif defined(__arm__)
    sysinfo.processor_architecture = MINIDUMP_CPU_ARM;
#    endif

#    if defined(SENTRY_PLATFORM_ANDROID)
    sysinfo.platform_id = MINIDUMP_OS_ANDROID;
#    else
    sysinfo.platform_id = MINIDUMP_OS_LINUX;
#    endif

    sysinfo.number_of_processors = (uint8_t)sysconf(_SC_NPROCESSORS_ONLN);

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

#    if defined(__aarch64__)
/**
 * Parse the __reserved field in mcontext to find FPSIMD context
 */
static const struct fpsimd_context *
find_fpsimd_context(const ucontext_t *uctx)
{
    // The __reserved field contains a chain of context blocks
    const uint8_t *ptr = (const uint8_t *)uctx->uc_mcontext.__reserved;
    const uint8_t *end = ptr + sizeof(uctx->uc_mcontext.__reserved);

    // Walk through context blocks looking for FPSIMD_MAGIC
    while (ptr + sizeof(struct _aarch64_ctx) <= end) {
        const struct _aarch64_ctx *ctx = (const struct _aarch64_ctx *)ptr;

        // Check for end marker (magic = 0, size = 0)
        if (ctx->magic == 0 && ctx->size == 0) {
            break;
        }

        // Check for valid size
        if (ctx->size == 0 || ctx->size > (size_t)(end - ptr)) {
            break;
        }

        // Found FPSIMD context
        if (ctx->magic == FPSIMD_MAGIC) {
            if (ctx->size >= sizeof(struct fpsimd_context)) {
                return (const struct fpsimd_context *)ctx;
            }
            break;
        }

        // Move to next context block
        ptr += ctx->size;
    }

    return NULL;
}
#    endif

/**
 * Convert Linux ucontext_t to minidump context
 */
static minidump_rva_t
write_thread_context(minidump_writer_t *writer, const ucontext_t *uctx)
{
    if (!uctx) {
        return 0;
    }

#    if defined(__x86_64__)
    minidump_context_x86_64_t context = { 0 };
    // Set flags for full context (control + integer + segments + floating
    // point)
    context.context_flags
        = 0x0010003f; // CONTEXT_AMD64 | CONTEXT_CONTROL | CONTEXT_INTEGER |
                      // CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT

    // Copy general purpose registers from Linux ucontext
    context.rax = uctx->uc_mcontext.gregs[REG_RAX];
    context.rbx = uctx->uc_mcontext.gregs[REG_RBX];
    context.rcx = uctx->uc_mcontext.gregs[REG_RCX];
    context.rdx = uctx->uc_mcontext.gregs[REG_RDX];
    context.rsi = uctx->uc_mcontext.gregs[REG_RSI];
    context.rdi = uctx->uc_mcontext.gregs[REG_RDI];
    context.rbp = uctx->uc_mcontext.gregs[REG_RBP];
    context.rsp = uctx->uc_mcontext.gregs[REG_RSP];
    context.r8 = uctx->uc_mcontext.gregs[REG_R8];
    context.r9 = uctx->uc_mcontext.gregs[REG_R9];
    context.r10 = uctx->uc_mcontext.gregs[REG_R10];
    context.r11 = uctx->uc_mcontext.gregs[REG_R11];
    context.r12 = uctx->uc_mcontext.gregs[REG_R12];
    context.r13 = uctx->uc_mcontext.gregs[REG_R13];
    context.r14 = uctx->uc_mcontext.gregs[REG_R14];
    context.r15 = uctx->uc_mcontext.gregs[REG_R15];
    context.rip = uctx->uc_mcontext.gregs[REG_RIP];
    context.eflags = uctx->uc_mcontext.gregs[REG_EFL];
    context.cs = uctx->uc_mcontext.gregs[REG_CSGSFS] & 0xffff;

    // Skip FPU state - the fpregs pointer is invalid in daemon process
    // For crashed thread: fpregs points to parent process memory (inaccessible)
    // For other threads: fpregs is never populated by our ptrace code
    // TODO: Add PTRACE_GETFPREGS support if FPU registers are needed
    // For now, general purpose registers are sufficient for stack unwinding

    return write_data(writer, &context, sizeof(context));

#    elif defined(__aarch64__)
    minidump_context_arm64_t context = { 0 };
    // Set flags for control + integer + fpsimd registers (FULL context)
    context.context_flags = 0x00400007; // ARM64 | Control | Integer | Fpsimd

    // Copy general purpose registers X0-X28
    for (int i = 0; i < 29; i++) {
        context.regs[i] = uctx->uc_mcontext.regs[i];
    }
    // Copy FP, LR, SP, PC separately
    context.fp = uctx->uc_mcontext.regs[29]; // X29
    context.lr = uctx->uc_mcontext.regs[30]; // X30
    context.sp = uctx->uc_mcontext.sp;
    context.pc = uctx->uc_mcontext.pc;
    context.cpsr = uctx->uc_mcontext.pstate;

    // Parse __reserved field to find FPSIMD context with NEON/FP registers
    const struct fpsimd_context *fpsimd = find_fpsimd_context(uctx);
    if (fpsimd) {
        // Copy NEON/FP registers V0-V31 from Linux __uint128_t to our
        // uint128_struct
        for (int i = 0; i < 32; i++) {
            __uint128_t vreg = fpsimd->vregs[i];
            context.fpsimd[i].low = (uint64_t)vreg;
            context.fpsimd[i].high = (uint64_t)(vreg >> 64);
        }
        context.fpsr = fpsimd->fpsr;
        context.fpcr = fpsimd->fpcr;
    } else {
        // FPSIMD context not found, zero out registers
        memset(context.fpsimd, 0, sizeof(context.fpsimd));
        context.fpsr = 0;
        context.fpcr = 0;
    }

    // Zero out debug registers
    memset(context.bcr, 0, sizeof(context.bcr));
    memset(context.bvr, 0, sizeof(context.bvr));
    memset(context.wcr, 0, sizeof(context.wcr));
    memset(context.wvr, 0, sizeof(context.wvr));

    return write_data(writer, &context, sizeof(context));

#    elif defined(__i386__)
    minidump_context_x86_t context = { 0 };
    // Set flags for control + integer + segments (no floating point in this
    // simplified struct)
    context.context_flags = 0x0001001f; // CONTEXT_i386 | CONTEXT_CONTROL |
                                        // CONTEXT_INTEGER | CONTEXT_SEGMENTS

    // Copy general purpose registers from Linux ucontext
    context.eax = uctx->uc_mcontext.gregs[REG_EAX];
    context.ebx = uctx->uc_mcontext.gregs[REG_EBX];
    context.ecx = uctx->uc_mcontext.gregs[REG_ECX];
    context.edx = uctx->uc_mcontext.gregs[REG_EDX];
    context.esi = uctx->uc_mcontext.gregs[REG_ESI];
    context.edi = uctx->uc_mcontext.gregs[REG_EDI];
    context.ebp = uctx->uc_mcontext.gregs[REG_EBP];
    context.esp = uctx->uc_mcontext.gregs[REG_ESP];
    context.eip = uctx->uc_mcontext.gregs[REG_EIP];
    context.eflags = uctx->uc_mcontext.gregs[REG_EFL];
    context.cs = uctx->uc_mcontext.gregs[REG_CS];
    context.ds = uctx->uc_mcontext.gregs[REG_DS];
    context.es = uctx->uc_mcontext.gregs[REG_ES];
    context.fs = uctx->uc_mcontext.gregs[REG_FS];
    context.gs = uctx->uc_mcontext.gregs[REG_GS];
    context.ss = uctx->uc_mcontext.gregs[REG_SS];

    // Debug registers - zero out (not available from ucontext)
    context.dr0 = 0;
    context.dr1 = 0;
    context.dr2 = 0;
    context.dr3 = 0;
    context.dr6 = 0;
    context.dr7 = 0;

    // Note: FPU state not included in this simplified i386 context structure
    // This is sufficient for stack unwinding and crash analysis

    return write_data(writer, &context, sizeof(context));

#    else
#        error "Unsupported architecture for Linux"
#    endif
}

/**
 * Extract Build ID from ELF file
 * Returns the Build ID length, or 0 if not found
 */
static size_t
extract_elf_build_id(const char *elf_path, uint8_t *build_id, size_t max_len)
{
    int fd = open(elf_path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    // Read ELF header
#    if defined(__x86_64__) || defined(__aarch64__)
    Elf64_Ehdr ehdr;
#    else
    Elf32_Ehdr ehdr;
#    endif

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        close(fd);
        return 0;
    }

    // Verify ELF magic
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        close(fd);
        return 0;
    }

    // Read section headers
    size_t shdr_size = ehdr.e_shentsize * ehdr.e_shnum;
    void *shdr_buf = sentry_malloc(shdr_size);
    if (!shdr_buf) {
        close(fd);
        return 0;
    }

    if (lseek(fd, ehdr.e_shoff, SEEK_SET) != (off_t)ehdr.e_shoff
        || read(fd, shdr_buf, shdr_size) != (ssize_t)shdr_size) {
        sentry_free(shdr_buf);
        close(fd);
        return 0;
    }

#    if defined(__x86_64__) || defined(__aarch64__)
    Elf64_Shdr *sections = (Elf64_Shdr *)shdr_buf;
#    else
    Elf32_Shdr *sections = (Elf32_Shdr *)shdr_buf;
#    endif

    // Look for .note.gnu.build-id section
    size_t build_id_len = 0;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (sections[i].sh_type == SHT_NOTE) {
            // Read note section
            size_t note_size = sections[i].sh_size;
            if (note_size > 4096)
                continue; // Sanity check

            void *note_buf = sentry_malloc(note_size);
            if (!note_buf)
                continue;

            if (lseek(fd, sections[i].sh_offset, SEEK_SET)
                    == (off_t)sections[i].sh_offset
                && read(fd, note_buf, note_size) == (ssize_t)note_size) {

                // Parse notes
                uint8_t *ptr = (uint8_t *)note_buf;
                uint8_t *end = ptr + note_size;

                while (ptr + 12 <= end) {
#    if defined(__x86_64__) || defined(__aarch64__)
                    Elf64_Nhdr *nhdr = (Elf64_Nhdr *)ptr;
#    else
                    Elf32_Nhdr *nhdr = (Elf32_Nhdr *)ptr;
#    endif
                    ptr += sizeof(*nhdr);

                    if (ptr + nhdr->n_namesz + nhdr->n_descsz > end)
                        break;

                    // Check if this is GNU Build ID (type 3, name "GNU\0")
                    if (nhdr->n_type == 3 && nhdr->n_namesz == 4
                        && memcmp(ptr, "GNU", 4) == 0) {

                        ptr += ((nhdr->n_namesz + 3) & ~3); // Align to 4 bytes
                        size_t len = nhdr->n_descsz < max_len ? nhdr->n_descsz
                                                              : max_len;
                        memcpy(build_id, ptr, len);
                        build_id_len = len;
                        sentry_free(note_buf);
                        goto done;
                    }

                    ptr += ((nhdr->n_namesz + 3) & ~3);
                    ptr += ((nhdr->n_descsz + 3) & ~3);
                }
            }

            sentry_free(note_buf);
        }
    }

done:
    sentry_free(shdr_buf);
    close(fd);
    return build_id_len;
}

/**
 * Write CodeView record with Build ID
 */
static minidump_rva_t
write_cv_record(minidump_writer_t *writer, const char *module_path,
    const uint8_t *build_id, size_t build_id_len)
{
    (void)module_path; // Not used in ELF format (only signature + build_id)

    if (!build_id || build_id_len == 0) {
        return 0;
    }

    // Calculate size: signature (4 bytes) + build_id (variable length)
    // Note: Breakpad's format is just signature + raw build_id bytes
    // No filename is stored in the CV record for ELF
    size_t total_size = sizeof(uint32_t) + build_id_len;

    uint8_t *cv_record = sentry_malloc(total_size);
    if (!cv_record) {
        return 0;
    }

    // Write 'BpEL' signature (0x4270454c)
    uint32_t signature = CV_SIGNATURE_ELF;
    memcpy(cv_record, &signature, sizeof(signature));

    // Write raw Build ID bytes (typically 20 bytes for SHA-1)
    memcpy(cv_record + sizeof(signature), build_id, build_id_len);

    SENTRY_DEBUGF(
        "CV Record: signature=0x%x, build_id_len=%zu", signature, build_id_len);

    minidump_rva_t rva = write_data(writer, cv_record, total_size);
    sentry_free(cv_record);
    return rva;
}

/**
 * Write UTF-16LE string for minidump
 */
static minidump_rva_t
write_minidump_string(minidump_writer_t *writer, const char *str)
{
    if (!str) {
        return 0;
    }

    size_t utf8_len = strlen(str);
    size_t utf16_len = utf8_len; // Approximate (ASCII chars = 1:1)

    // Allocate buffer for UTF-16LE string (including null terminator)
    uint32_t total_size
        = sizeof(uint32_t) + (utf16_len * 2) + 2; // +2 for null terminator
    uint8_t *buf = sentry_malloc(total_size);
    if (!buf) {
        return 0;
    }

    // Write string length (in bytes, NOT including null terminator)
    uint32_t string_bytes = utf16_len * 2;
    memcpy(buf, &string_bytes, sizeof(uint32_t));

    // Convert UTF-8 to UTF-16LE (simple ASCII conversion)
    uint16_t *utf16 = (uint16_t *)(buf + sizeof(uint32_t));
    for (size_t i = 0; i < utf8_len; i++) {
        utf16[i] = (uint16_t)(unsigned char)str[i];
    }
    utf16[utf8_len] = 0; // Null terminator

    minidump_rva_t rva = write_data(writer, buf, total_size);
    sentry_free(buf);
    return rva;
}

/**
 * Write stack memory for a thread
 * Returns RVA to stack data, and sets stack_size_out and stack_start_out
 */
static minidump_rva_t
write_thread_stack(minidump_writer_t *writer, uint64_t stack_pointer,
    size_t *stack_size_out, uint64_t *stack_start_out)
{
    SENTRY_DEBUGF(
        "write_thread_stack: SP=0x%llx", (unsigned long long)stack_pointer);

    // On x86_64, include the red zone (128 bytes below SP)
    // Leaf functions can use this area without adjusting SP
#    if defined(__x86_64__)
    const size_t RED_ZONE = 128;
    uint64_t capture_start
        = stack_pointer >= RED_ZONE ? stack_pointer - RED_ZONE : stack_pointer;
#    else
    uint64_t capture_start = stack_pointer;
#    endif

    // Find the stack mapping for this thread
    uint64_t stack_start = 0;
    uint64_t stack_end = 0;

    for (size_t i = 0; i < writer->mapping_count; i++) {
        if (stack_pointer >= writer->mappings[i].start
            && stack_pointer < writer->mappings[i].end
            && strstr(writer->mappings[i].name, "[stack") != NULL) {
            stack_start = writer->mappings[i].start;
            stack_end = writer->mappings[i].end;
            break;
        }
    }

    if (stack_start == 0) {
        // Stack mapping not found, use a reasonable range
        const size_t DEFAULT_STACK_SIZE = SENTRY_CRASH_MAX_STACK_CAPTURE;
        stack_start = capture_start;
        stack_end = stack_pointer + DEFAULT_STACK_SIZE;
    }

    // Ensure capture_start is within stack bounds
    if (capture_start < stack_start) {
        capture_start = stack_start;
    }

    // Capture from adjusted SP to end of stack (upwards)
    size_t stack_size = stack_end - capture_start;

    // Limit to 1MB
    if (stack_size > SENTRY_CRASH_MAX_STACK_SIZE) {
        stack_size = SENTRY_CRASH_MAX_STACK_SIZE;
    }

    void *stack_buffer = sentry_malloc(stack_size);
    if (!stack_buffer) {
        *stack_size_out = 0;
        return 0;
    }

    // Read stack memory from crashed process (including red zone if applicable)
    ssize_t nread
        = read_process_memory(writer, capture_start, stack_buffer, stack_size);

    minidump_rva_t rva = 0;
    if (nread > 0) {
        rva = write_data(writer, stack_buffer, nread);
        *stack_size_out = nread;
        *stack_start_out = capture_start; // Return the actual start address
        SENTRY_DEBUGF(
            "Read %zd bytes of stack memory from 0x%llx (SP was 0x%llx)", nread,
            (unsigned long long)capture_start,
            (unsigned long long)stack_pointer);
    } else {
        SENTRY_WARNF(
            "Failed to read stack memory from process %d at 0x%llx (size %zu): "
            "%s",
            writer->crash_ctx->crashed_pid, (unsigned long long)capture_start,
            stack_size, strerror(errno));
        *stack_size_out = 0;
        *stack_start_out = 0;
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
    SENTRY_DEBUGF(
        "write_thread_list_stream: %zu threads", writer->thread_count);

    // Calculate total size needed
    size_t list_size
        = sizeof(uint32_t) + (writer->thread_count * sizeof(minidump_thread_t));

    minidump_thread_list_t *thread_list = sentry_malloc(list_size);
    if (!thread_list) {
        SENTRY_WARN("Failed to allocate thread list");
        return -1;
    }

    thread_list->count = writer->thread_count;

    // Fill in thread info with context and stack
    for (size_t i = 0; i < writer->thread_count; i++) {
        SENTRY_DEBUGF("Processing thread %zu/%zu (tid=%d)", i + 1,
            writer->thread_count, writer->tids[i]);

        minidump_thread_t *thread = &thread_list->threads[i];
        memset(thread, 0, sizeof(*thread));

        thread->thread_id = writer->tids[i];

        // Try to find this thread in the captured threads
        const ucontext_t *uctx = NULL;
        for (size_t j = 0; j < writer->crash_ctx->platform.num_threads; j++) {
            if (writer->crash_ctx->platform.threads[j].tid == writer->tids[i]) {
                uctx = &writer->crash_ctx->platform.threads[j].context;
                break;
            }
        }

        // If we have context for this thread, write it
        if (uctx) {
            SENTRY_DEBUGF("Thread %u: writing context", thread->thread_id);
            // Write thread context
            thread->thread_context.rva = write_thread_context(writer, uctx);
            thread->thread_context.size = get_context_size();
            SENTRY_DEBUGF("Thread %u: context written at RVA 0x%x",
                thread->thread_id, thread->thread_context.rva);

            // Write stack memory
            uint64_t sp;
#    if defined(__x86_64__)
            sp = uctx->uc_mcontext.gregs[REG_RSP];
#    elif defined(__aarch64__)
            sp = uctx->uc_mcontext.sp;
#    elif defined(__i386__)
            sp = uctx->uc_mcontext.gregs[REG_ESP];
#    endif

            SENTRY_DEBUGF("Thread %u: has context, SP=0x%llx",
                thread->thread_id, (unsigned long long)sp);

            if (sp != 0) {
                size_t stack_size = 0;
                uint64_t stack_start = 0;
                thread->stack.memory.rva
                    = write_thread_stack(writer, sp, &stack_size, &stack_start);
                thread->stack.memory.size = stack_size;
                thread->stack.start_address = stack_start;

                SENTRY_DEBUGF("Thread %u: wrote context at RVA 0x%x, stack at "
                              "RVA 0x%x (size %zu)",
                    thread->thread_id, thread->thread_context.rva,
                    thread->stack.memory.rva, stack_size);
            } else {
                // SP is 0, try to get registers via ptrace
                SENTRY_DEBUGF(
                    "Thread %u: SP is 0, attempting to capture via ptrace",
                    thread->thread_id);

                ucontext_t ptrace_ctx;
                memset(&ptrace_ctx, 0, sizeof(ptrace_ctx));

                if (ptrace_get_thread_registers(
                        thread->thread_id, &ptrace_ctx)) {
                    // Successfully got registers, update context and re-write
                    // it
                    SENTRY_DEBUGF("Thread %u: successfully captured via ptrace",
                        thread->thread_id);

                    // Re-write the thread context with the captured registers
                    thread->thread_context.rva
                        = write_thread_context(writer, &ptrace_ctx);

                    // Extract SP from captured context
                    uint64_t ptrace_sp;
#    if defined(__x86_64__)
                    ptrace_sp = ptrace_ctx.uc_mcontext.gregs[REG_RSP];
#    elif defined(__aarch64__)
                    ptrace_sp = ptrace_ctx.uc_mcontext.sp;
#    elif defined(__i386__)
                    ptrace_sp = ptrace_ctx.uc_mcontext.gregs[REG_ESP];
#    endif

                    if (ptrace_sp != 0) {
                        size_t stack_size = 0;
                        uint64_t stack_start = 0;
                        thread->stack.memory.rva = write_thread_stack(
                            writer, ptrace_sp, &stack_size, &stack_start);
                        thread->stack.memory.size = stack_size;
                        thread->stack.start_address = stack_start;

                        SENTRY_DEBUGF("Thread %u: wrote ptrace context at RVA "
                                      "0x%x, stack at "
                                      "RVA 0x%x (size %zu)",
                            thread->thread_id, thread->thread_context.rva,
                            thread->stack.memory.rva, stack_size);
                    }
                } else {
                    SENTRY_WARNF("Thread %u: failed to capture via ptrace",
                        thread->thread_id);
                }
            }
        } else {
            SENTRY_DEBUGF("Thread %u: no context available", thread->thread_id);
        }
    }

    dir->stream_type = MINIDUMP_STREAM_THREAD_LIST;
    dir->rva = write_data(writer, thread_list, list_size);
    dir->data_size = list_size;

    sentry_free(thread_list);
    return dir->rva ? 0 : -1;
}

/**
 * Write module list stream (shared libraries)
 */
static int
write_module_list_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    SENTRY_DEBUGF("write_module_list_stream: processing %zu total mappings",
        writer->mapping_count);

    // Count modules (mappings with executable flag and name)
    size_t module_count = 0;
    for (size_t i = 0; i < writer->mapping_count; i++) {
        if (writer->mappings[i].permissions[2] == 'x'
            && writer->mappings[i].name[0] != '\0'
            && writer->mappings[i].name[0] != '[') {
            module_count++;
        }
    }

    size_t list_size
        = sizeof(uint32_t) + (module_count * sizeof(minidump_module_t));
    minidump_module_list_t *module_list = sentry_malloc(list_size);
    if (!module_list) {
        return -1;
    }

    module_list->count = module_count;
    SENTRY_DEBUGF("Writing %zu modules to minidump", module_count);

    // First pass: collect module info and Build IDs (don't write anything yet)
    typedef struct {
        uint8_t build_id[32];
        size_t build_id_len;
        char *name;
        uint64_t base;
        uint32_t size;
    } module_info_t;
    module_info_t *mod_infos
        = sentry_malloc(sizeof(module_info_t) * module_count);
    if (!mod_infos) {
        sentry_free(module_list);
        return -1;
    }

    size_t mod_idx = 0;
    for (size_t i = 0; i < writer->mapping_count && mod_idx < module_count;
        i++) {
        memory_mapping_t *mapping = &writer->mappings[i];

        if (mapping->permissions[2] == 'x' && mapping->name[0] != '\0'
            && mapping->name[0] != '[') {
            minidump_module_t *module = &module_list->modules[mod_idx];
            memset(module, 0, sizeof(*module));

            module->base_of_image = mapping->start;
            module->size_of_image = mapping->end - mapping->start;

            // Set VS_FIXEDFILEINFO signature (first uint32_t of version_info)
            // This is required for minidump processors to recognize the module
            uint32_t version_sig = 0xFEEF04BD;
            memcpy(&module->version_info[0], &version_sig, sizeof(version_sig));

            // Store info for later writing
            mod_infos[mod_idx].name = mapping->name;
            mod_infos[mod_idx].base = mapping->start;
            mod_infos[mod_idx].size = mapping->end - mapping->start;

            // Extract Build ID but don't write anything yet
            mod_infos[mod_idx].build_id_len = extract_elf_build_id(
                mapping->name, mod_infos[mod_idx].build_id,
                sizeof(mod_infos[mod_idx].build_id));

            SENTRY_DEBUGF("Module: %s base=0x%llx size=0x%llx build_id_len=%zu",
                mapping->name, (unsigned long long)mapping->start,
                (unsigned long long)(mapping->end - mapping->start),
                mod_infos[mod_idx].build_id_len);

            mod_idx++;
        }
    }

    // Write the module list structure FIRST (with zero RVAs)
    dir->stream_type = MINIDUMP_STREAM_MODULE_LIST;
    dir->rva = write_data(writer, module_list, list_size);
    dir->data_size = list_size;

    // Second pass: write module names and CV records, then update module list
    for (size_t i = 0; i < module_count; i++) {
        // Write module name
        minidump_rva_t name_rva
            = write_minidump_string(writer, mod_infos[i].name);

        // Write CV record if we have a Build ID
        minidump_rva_t cv_rva = 0;
        uint32_t cv_size = 0;
        if (mod_infos[i].build_id_len > 0) {
            cv_rva = write_cv_record(
                writer, "", mod_infos[i].build_id, mod_infos[i].build_id_len);
            cv_size = sizeof(uint32_t) + mod_infos[i].build_id_len;
            SENTRY_DEBUGF("CV Record: signature=0x4270454c, build_id_len=%zu",
                mod_infos[i].build_id_len);
        }

        // Third pass: update specific fields in the module structure via lseek
        // Save position AFTER writing name and CV record
        off_t saved_pos = lseek(writer->fd, 0, SEEK_CUR);

        // Update module_name_rva field
        off_t name_rva_offset = dir->rva + sizeof(uint32_t)
            + (i * sizeof(minidump_module_t))
            + offsetof(minidump_module_t, module_name_rva);

        if (lseek(writer->fd, name_rva_offset, SEEK_SET)
            == (off_t)name_rva_offset) {
            if (write(writer->fd, &name_rva, sizeof(name_rva))
                != sizeof(name_rva)) {
                SENTRY_WARNF(
                    "Failed to write module_name_rva for module %zu", i);
            }
        }

        // Update cv_record fields (size and rva)
        if (cv_size > 0) {
            off_t cv_offset = dir->rva + sizeof(uint32_t)
                + (i * sizeof(minidump_module_t))
                + offsetof(minidump_module_t, cv_record);

            SENTRY_DEBUGF("  Seeking to CV offset: 0x%llx for module %zu",
                (unsigned long long)cv_offset, i);

            off_t actual_offset = lseek(writer->fd, cv_offset, SEEK_SET);
            if (actual_offset == (off_t)cv_offset) {
                // Write size first, then rva (order in structure)
                ssize_t written1 = write(writer->fd, &cv_size, sizeof(cv_size));
                ssize_t written2 = write(writer->fd, &cv_rva, sizeof(cv_rva));

                if (written1 == sizeof(cv_size) && written2 == sizeof(cv_rva)) {
                    // Force flush to disk
                    fsync(writer->fd);
                    SENTRY_DEBUGF(
                        "  Updated module[%zu]: name_rva=0x%x, cv_rva=0x%x, "
                        "cv_size=%u (flushed)",
                        i, name_rva, cv_rva, cv_size);
                } else {
                    SENTRY_WARNF("Failed to write CV record for module %zu: "
                                 "written1=%zd, written2=%zd",
                        i, written1, written2);
                }
            } else {
                SENTRY_WARNF("Failed to seek to CV offset 0x%llx for module "
                             "%zu (got 0x%llx)",
                    (unsigned long long)cv_offset, i,
                    (unsigned long long)actual_offset);
            }
        }

        lseek(writer->fd, saved_pos, SEEK_SET);
    }

    // Final flush to ensure all writes are committed
    fsync(writer->fd);
    SENTRY_DEBUG("Flushed all module updates to disk");

    sentry_free(mod_infos);

    sentry_free(module_list);
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

    // Map signal to exception code
    exception_stream.exception_record.exception_code
        = 0x40000000 | writer->crash_ctx->platform.signum;
    exception_stream.exception_record.exception_flags = 0;
    exception_stream.exception_record.exception_address
        = (uint64_t)writer->crash_ctx->platform.siginfo.si_addr;
    exception_stream.exception_record.number_parameters = 0;

    // Write the crashing thread's context
    const ucontext_t *uctx = &writer->crash_ctx->platform.context;
    exception_stream.thread_context.rva = write_thread_context(writer, uctx);
    exception_stream.thread_context.size = get_context_size();

    SENTRY_DEBUGF("Exception: wrote context at RVA 0x%x for thread %u",
        exception_stream.thread_context.rva, exception_stream.thread_id);

    dir->stream_type = MINIDUMP_STREAM_EXCEPTION;
    dir->rva = write_data(writer, &exception_stream, sizeof(exception_stream));
    dir->data_size = sizeof(exception_stream);

    return dir->rva ? 0 : -1;
}

/**
 * Check if a memory region should be included based on minidump mode
 */
static bool
should_include_region(const memory_mapping_t *mapping,
    sentry_minidump_mode_t mode, uint64_t crash_addr)
{
    // STACK_ONLY: Only include stack regions (captured in thread list already)
    if (mode == SENTRY_MINIDUMP_MODE_STACK_ONLY) {
        return false; // Thread list already has stack memory
    }

    // FULL: Include all readable regions
    if (mode == SENTRY_MINIDUMP_MODE_FULL) {
        return mapping->permissions[0] == 'r'; // Must be readable
    }

    // SMART: Include heap regions near crash address, and special regions
    if (mode == SENTRY_MINIDUMP_MODE_SMART) {
        // Include regions containing crash address
        if (crash_addr >= mapping->start && crash_addr < mapping->end) {
            return mapping->permissions[0] == 'r';
        }

        // Include heap regions (likely named [heap] or anonymous with rw-)
        if (strstr(mapping->name, "[heap]") != NULL) {
            return mapping->permissions[0] == 'r';
        }

        // Include writable anonymous regions (likely heap allocations)
        if (mapping->name[0] == '\0' && mapping->permissions[0] == 'r'
            && mapping->permissions[1] == 'w') {
            // Limit to reasonable size to avoid huge dumps (max 64MB per
            // region)
            return (mapping->end - mapping->start)
                <= (64 * SENTRY_CRASH_MAX_STACK_SIZE);
        }
    }

    return false;
}

/**
 * Write memory list stream (heap memory based on minidump mode)
 */
static int
write_memory_list_stream(minidump_writer_t *writer, minidump_directory_t *dir)
{
    // Get crash address for SMART mode filtering
    uint64_t crash_addr = (uint64_t)writer->crash_ctx->platform.siginfo.si_addr;

    // Count regions to include based on mode
    size_t region_count = 0;
    for (size_t i = 0; i < writer->mapping_count; i++) {
        if (should_include_region(&writer->mappings[i],
                writer->crash_ctx->minidump_mode, crash_addr)) {
            region_count++;
        }
    }

    // Allocate memory list
    size_t list_size = sizeof(uint32_t)
        + (region_count * sizeof(minidump_memory_descriptor_t));
    minidump_memory_list_t *memory_list = sentry_malloc(list_size);
    if (!memory_list) {
        return -1;
    }

    memory_list->count = region_count;

    // Write memory regions
    size_t mem_idx = 0;
    for (size_t i = 0; i < writer->mapping_count && mem_idx < region_count;
        i++) {
        if (!should_include_region(&writer->mappings[i],
                writer->crash_ctx->minidump_mode, crash_addr)) {
            continue;
        }

        memory_mapping_t *mapping = &writer->mappings[i];
        minidump_memory_descriptor_t *mem = &memory_list->ranges[mem_idx++];

        uint64_t region_size = mapping->end - mapping->start;

        // Limit individual region size to avoid huge dumps
        const size_t MAX_REGION_SIZE = 64 * SENTRY_CRASH_MAX_STACK_SIZE; // 64MB
        if (region_size > MAX_REGION_SIZE) {
            region_size = MAX_REGION_SIZE;
        }

        // Allocate buffer for region memory
        void *region_buffer = sentry_malloc(region_size);
        if (!region_buffer) {
            mem->start_address = mapping->start;
            mem->memory.size = 0;
            mem->memory.rva = 0;
            continue;
        }

        // Read memory from crashed process
        ssize_t nread = read_process_memory(
            writer, mapping->start, region_buffer, region_size);

        if (nread > 0) {
            mem->start_address = mapping->start;
            mem->memory.rva = write_data(writer, region_buffer, nread);
            mem->memory.size = nread;
        } else {
            mem->start_address = mapping->start;
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
 * Main minidump writing function for Linux
 */
int
sentry__write_minidump(
    const sentry_crash_context_t *ctx, const char *output_path)
{
    SENTRY_DEBUGF("writing minidump to %s", output_path);
    SENTRY_DEBUGF("crashed_pid=%d, crashed_tid=%d, num_threads=%zu",
        ctx->crashed_pid, ctx->crashed_tid, ctx->platform.num_threads);

    minidump_writer_t writer = { 0 };
    writer.crash_ctx = ctx;

    // Open output file
    writer.fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (writer.fd < 0) {
        SENTRY_WARNF("failed to create minidump: %s", strerror(errno));
        return -1;
    }

    // Parse process information
    if (parse_proc_maps(&writer) < 0 || enumerate_threads(&writer) < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Reserve space for header and directory
    // Number of streams depends on minidump mode:
    // - STACK_ONLY: 4 streams (no memory list)
    // - SMART/FULL: 5 streams (with memory list)
    const uint32_t stream_count
        = (ctx->minidump_mode == SENTRY_MINIDUMP_MODE_STACK_ONLY) ? 4 : 5;
    writer.current_offset = sizeof(minidump_header_t)
        + (stream_count * sizeof(minidump_directory_t));

    SENTRY_DEBUGF("reserving space for %u streams, offset=%zu", stream_count,
        writer.current_offset);

    if (lseek(writer.fd, writer.current_offset, SEEK_SET) < 0) {
        SENTRY_WARN("lseek failed");
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Write streams
    minidump_directory_t directories[5];
    int result = 0;

    SENTRY_DEBUG("writing system info stream");
    result |= write_system_info_stream(&writer, &directories[0]);
    SENTRY_DEBUG("writing thread list stream");
    result |= write_thread_list_stream(&writer, &directories[1]);
    SENTRY_DEBUG("writing module list stream");
    result |= write_module_list_stream(&writer, &directories[2]);
    SENTRY_DEBUG("writing exception stream");
    result |= write_exception_stream(&writer, &directories[3]);

    // Write memory list stream for SMART and FULL modes
    if (stream_count == 5) {
        result |= write_memory_list_stream(&writer, &directories[4]);
    }

    if (result < 0) {
        close(writer.fd);
        unlink(output_path);
        return -1;
    }

    // Write header and directory at the beginning
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

    close(writer.fd);

    // Detach from process if we attached
    if (writer.ptrace_attached) {
        ptrace(PTRACE_DETACH, ctx->crashed_pid, NULL, NULL);
        SENTRY_DEBUGF("Detached from process %d", ctx->crashed_pid);
    }

    SENTRY_DEBUG("successfully wrote minidump");
    return 0;
}

#endif // SENTRY_PLATFORM_LINUX || SENTRY_PLATFORM_ANDROID
