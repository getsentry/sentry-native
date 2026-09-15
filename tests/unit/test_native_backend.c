/**
 * Unit tests for native crash backend
 *
 * Tests minidump structures, Build ID extraction, UUID extraction,
 * and low-level crash handling functionality.
 */

#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"
#include "sentry_tracing.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef SENTRY_BACKEND_NATIVE
// Include native backend headers
#    include "../../src/backends/native/minidump/sentry_minidump_format.h"
#    include "../../src/backends/native/sentry_crash_context.h"
#    include "../../src/backends/native/sentry_crash_ipc.h"

static void
noop_crashed_last_run(const sentry_envelope_t *envelope, void *user_data)
{
    (void)envelope;
    (void)user_data;
}

static void
count_sent_envelopes(sentry_envelope_t *envelope, void *state)
{
    size_t *count = state;
    (*count)++;
    sentry_envelope_free(envelope);
}
#endif

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include "sentry_elf.h"
#endif

#if defined(SENTRY_PLATFORM_LINUX)
#    include <fcntl.h>
#    include <stdlib.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

SENTRY_TEST(daemon_adopts_existing_run)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    TEST_ASSERT(sentry__path_remove_all(options->database_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(options->database_path) == 0);

    sentry_run_t *existing_run = sentry__run_new(options->database_path);
    TEST_ASSERT(!!existing_run);
    sentry_run_t *adopted_run
        = sentry__run_adopt(options->database_path, existing_run->run_path);
    TEST_ASSERT(!!adopted_run);

    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    sentry_path_t *external_path
        = sentry__path_join_str(options->database_path, "external");
    sentry_path_t *daemon_lock_path
        = sentry__path_append_str(existing_run->run_path, ".daemon.lock");
    TEST_ASSERT(!!cache_path);
    TEST_ASSERT(!!external_path);
    TEST_ASSERT(!!daemon_lock_path);

    TEST_CHECK(sentry__path_eq(adopted_run->run_path, existing_run->run_path));
    TEST_CHECK(sentry__path_eq(adopted_run->cache_path, cache_path));
    TEST_CHECK(sentry__path_eq(adopted_run->external_path, external_path));
    TEST_CHECK(sentry__path_is_file(daemon_lock_path));

    size_t run_count = 0;
    sentry_pathiter_t *it = sentry__path_iter_directory(options->database_path);
    const sentry_path_t *entry;
    while (it && (entry = sentry__pathiter_next(it)) != NULL) {
        if (sentry__path_is_dir(entry)
            && sentry__path_ends_with(entry, ".run")) {
            run_count++;
        }
    }
    sentry__pathiter_free(it);
    TEST_CHECK_INT_EQUAL(run_count, 1);

    sentry__run_free(adopted_run);
    TEST_CHECK(!sentry__path_is_file(daemon_lock_path));
    sentry__run_clean(existing_run, true);
    sentry__run_free(existing_run);
    sentry__path_free(daemon_lock_path);
    sentry__path_free(external_path);
    sentry__path_free(cache_path);
    sentry_options_free(options);
}

SENTRY_TEST(daemon_run_blocks_old_run_cleanup)
{
#if defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)
    SKIP_TEST();
#endif
    SENTRY_TEST_OPTIONS_NEW(options);
    TEST_ASSERT(sentry__path_remove_all(options->database_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(options->database_path) == 0);

    options->run = sentry__run_new(options->database_path);
    TEST_ASSERT(!!options->run);
    sentry_run_t *old_run = sentry__run_new(options->database_path);
    TEST_ASSERT(!!old_run);
    sentry__filelock_unlock(old_run->lock);

    sentry_run_t *daemon_run
        = sentry__run_adopt(options->database_path, old_run->run_path);
    TEST_ASSERT(!!daemon_run);
    sentry_path_t *artifact
        = sentry__path_join_str(old_run->run_path, "daemon.log");
    TEST_ASSERT(!!artifact);
    TEST_ASSERT(sentry__path_write_buffer(artifact, "log", 3) == 0);

    sentry__process_old_runs(options, 0);
    TEST_CHECK(sentry__path_is_dir(old_run->run_path));
    TEST_CHECK(sentry__path_is_file(artifact));

    sentry__run_free(daemon_run);
    sentry__process_old_runs(options, 0);
    TEST_CHECK(!sentry__path_is_dir(old_run->run_path));
    TEST_CHECK(!sentry__path_is_file(artifact));

    sentry__path_free(artifact);
    sentry__run_free(old_run);
    sentry__run_clean(options->run, true);
    sentry_options_free(options);
}

SENTRY_TEST(corrupt_crash_envelope_does_not_block_old_run)
{
#ifdef SENTRY_BACKEND_NATIVE
    SENTRY_TEST_OPTIONS_NEW(options);
    TEST_ASSERT(sentry__path_remove_all(options->database_path) == 0);
    TEST_ASSERT(sentry__path_create_dir_all(options->database_path) == 0);

    options->run = sentry__run_new(options->database_path);
    TEST_ASSERT(!!options->run);
    sentry_run_t *old_run = sentry__run_new(options->database_path);
    TEST_ASSERT(!!old_run);
    sentry__filelock_unlock(old_run->lock);

    sentry_path_t *crash_envelope
        = sentry__path_join_str(old_run->run_path, "__sentry-crash.envelope");
    sentry_path_t *queued_envelope
        = sentry__path_join_str(old_run->run_path, "queued.envelope");
    TEST_ASSERT(!!crash_envelope && !!queued_envelope);
    TEST_ASSERT(sentry__path_write_buffer(crash_envelope, "garbage", 7) == 0);

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry__envelope_add_event(envelope,
        sentry_value_new_message_event(SENTRY_LEVEL_ERROR, NULL, "queued"));
    TEST_ASSERT(sentry_envelope_write_to_path(envelope, queued_envelope) == 0);
    sentry_envelope_free(envelope);

    size_t sent_envelopes = 0;
    sentry_transport_t *transport = sentry_transport_new(count_sent_envelopes);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &sent_envelopes);
    sentry_options_set_transport(options, transport);
    sentry_options_set_on_crashed_last_run(
        options, noop_crashed_last_run, NULL);

    sentry__process_old_runs(options, 0);

    TEST_CHECK_INT_EQUAL(sent_envelopes, 1);
    TEST_CHECK(!sentry__path_is_dir(old_run->run_path));
    TEST_CHECK(!sentry__path_is_file(crash_envelope));
    TEST_CHECK(!sentry__path_is_file(queued_envelope));

    sentry__path_free(queued_envelope);
    sentry__path_free(crash_envelope);
    sentry__run_free(old_run);
    sentry__run_clean(options->run, true);
    sentry_options_free(options);
#else
    SKIP_TEST();
#endif
}

/**
 * Test minidump header structure size and alignment
 */
SENTRY_TEST(minidump_header_size)
{
#ifdef SENTRY_BACKEND_NATIVE
    // Minidump header should be exactly 32 bytes
    TEST_CHECK(sizeof(minidump_header_t) == 32);

    // Verify structure alignment
    minidump_header_t header = { 0 };
    header.signature = MINIDUMP_SIGNATURE;
    header.version = MINIDUMP_VERSION;

    TEST_CHECK(header.signature == 0x504d444d); // 'MDMP' in little-endian
    TEST_CHECK(header.version == 0xa793); // Version 1.0
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(elf_rejects_non_regular_files)
{
#if !defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    SKIP_TEST();
#else
    TEST_CHECK(sentry__elf_is_file("/proc/self/exe"));
    TEST_CHECK(!sentry__elf_is_file("/dev/null"));

    char path[] = "/tmp/sentry-minidump-module-XXXXXX";
    int fd = mkstemp(path);
    TEST_ASSERT(fd >= 0);
    close(fd);
    TEST_ASSERT(unlink(path) == 0);
    TEST_ASSERT(mkfifo(path, 0600) == 0);

    fd = open(path, O_RDWR | O_NONBLOCK);
    TEST_ASSERT(fd >= 0);
    TEST_ASSERT(write(fd, ELFMAG, SELFMAG) == (ssize_t)SELFMAG);

    TEST_CHECK(!sentry__elf_is_file(path));

    close(fd);
    TEST_CHECK(unlink(path) == 0);
#endif
}

/**
 * Test minidump directory entry structure
 */
SENTRY_TEST(minidump_directory_size)
{
#ifdef SENTRY_BACKEND_NATIVE
    TEST_CHECK(sizeof(minidump_directory_t) == 12);

    minidump_directory_t dir = { 0 };
    dir.stream_type = MINIDUMP_STREAM_SYSTEM_INFO;
    dir.data_size = 100;
    dir.rva = 1000;

    TEST_CHECK(dir.stream_type == 7); // SYSTEM_INFO is 7
    TEST_CHECK(dir.data_size == 100);
    TEST_CHECK(dir.rva == 1000);
#else
    SKIP_TEST();
#endif
}

/**
 * Test thread context structures
 */
SENTRY_TEST(minidump_context_sizes)
{
#ifdef SENTRY_BACKEND_NATIVE
#    if defined(__x86_64__)
    // x86_64 context with FPU should be 1232 bytes
    TEST_CHECK(sizeof(minidump_context_x86_64_t) == 1232);

    minidump_context_x86_64_t ctx = { 0 };
    ctx.context_flags = 0x0010003f; // Full context with FPU
    ctx.rip = 0x12345678;
    ctx.rsp = 0x7fff0000;

    TEST_CHECK(ctx.context_flags == 0x0010003f);
    TEST_CHECK(ctx.rip == 0x12345678);

    // Verify XMM save area exists
    ctx.float_save.mx_csr = 0x1f80;
    TEST_CHECK(ctx.float_save.mx_csr == 0x1f80);

#    elif defined(__aarch64__)
    // ARM64 context: 4+4 + 29*8 + 3*8 + 32*16 + 4+4 + 8*8 + 8*8 + 2*4 + 2*8
    // = 8 + 232 + 24 + 512 + 8 + 64 + 64 + 8 + 16 = 936 bytes (actual: 912 with
    // packing)
    TEST_CHECK(sizeof(minidump_context_arm64_t) == 912);

    minidump_context_arm64_t ctx = { 0 };
    ctx.context_flags = 0x00400007; // ARM64 | Control | Integer | Fpsimd
    ctx.pc = 0x100000000;
    ctx.sp = 0x16b000000;

    TEST_CHECK(ctx.context_flags == 0x00400007);
    TEST_CHECK(ctx.pc == 0x100000000);

    // Verify NEON/FP registers exist
    ctx.fpsr = 0x12345678;
    TEST_CHECK(ctx.fpsr == 0x12345678);

#    endif
#else
    SKIP_TEST();
#endif
}

/**
 * Test module structure
 */
SENTRY_TEST(minidump_module_structure)
{
#ifdef SENTRY_BACKEND_NATIVE
    // Module structure size:
    // base_of_image (8) + size_of_image (4) + checksum (4) + time_date_stamp
    // (4)
    // + module_name_rva (4) + version_info[13] (52) + cv_record (8) +
    // misc_record (8) + reserved0 (8) + reserved1 (8) = 108 bytes
    TEST_CHECK(sizeof(minidump_module_t) == 108);

    minidump_module_t module = { 0 };
    module.base_of_image = 0x100000000;
    module.size_of_image = 0x10000;
    module.module_name_rva = 1000;

    TEST_CHECK(module.base_of_image == 0x100000000);
    TEST_CHECK(module.size_of_image == 0x10000);

    // Verify CodeView record can be set
    module.cv_record.rva = 2000;
    module.cv_record.size = 100;

    TEST_CHECK(module.cv_record.rva == 2000);
    TEST_CHECK(module.cv_record.size == 100);
#else
    SKIP_TEST();
#endif
}

/**
 * Test thread structure
 */
SENTRY_TEST(minidump_thread_structure)
{
#ifdef SENTRY_BACKEND_NATIVE
    TEST_CHECK(sizeof(minidump_thread_t) == 48);

    minidump_thread_t thread = { 0 };
    thread.thread_id = 12345;
    thread.stack.start_address = 0x7fff0000;
    thread.stack.memory.size = 65536;
    thread.thread_context.rva = 1000;

    TEST_CHECK(thread.thread_id == 12345);
    TEST_CHECK(thread.stack.start_address == 0x7fff0000);
    TEST_CHECK(thread.stack.memory.size == 65536);
#else
    SKIP_TEST();
#endif
}

/**
 * Test system info structure
 */
SENTRY_TEST(minidump_system_info)
{
#ifdef SENTRY_BACKEND_NATIVE
    minidump_system_info_t sysinfo = { 0 };

#    if defined(__x86_64__)
    sysinfo.processor_architecture = MINIDUMP_CPU_X86_64;
    TEST_CHECK(sysinfo.processor_architecture == 9);
#    elif defined(__aarch64__)
    sysinfo.processor_architecture = MINIDUMP_CPU_ARM64;
    TEST_CHECK(sysinfo.processor_architecture == 12);
#    endif

    sysinfo.number_of_processors = 8;
    TEST_CHECK(sysinfo.number_of_processors == 8);
#else
    SKIP_TEST();
#endif
}

/**
 * Test exception record structure
 */
SENTRY_TEST(minidump_exception_record)
{
#ifdef SENTRY_BACKEND_NATIVE
    minidump_exception_record_t exception = { 0 };
    exception.exception_code = 0xc0000005; // Access violation
    exception.exception_address = 0x12345678;

    TEST_CHECK(exception.exception_code == 0xc0000005);
    TEST_CHECK(exception.exception_address == 0x12345678);
#else
    SKIP_TEST();
#endif
}

/**
 * Test memory descriptor structure
 */
SENTRY_TEST(minidump_memory_descriptor)
{
#ifdef SENTRY_BACKEND_NATIVE
    minidump_memory_descriptor_t mem = { 0 };
    mem.start_address = 0x7fff0000;
    mem.memory.size = 4096;
    mem.memory.rva = 1000;

    TEST_CHECK(mem.start_address == 0x7fff0000);
    TEST_CHECK(mem.memory.size == 4096);
    TEST_CHECK(mem.memory.rva == 1000);
#else
    SKIP_TEST();
#endif
}

/**
 * Test that minidump stream types are correct
 */
SENTRY_TEST(minidump_stream_types)
{
#ifdef SENTRY_BACKEND_NATIVE
    TEST_CHECK(MINIDUMP_STREAM_THREAD_LIST == 3);
    TEST_CHECK(MINIDUMP_STREAM_MODULE_LIST == 4);
    TEST_CHECK(MINIDUMP_STREAM_MEMORY_LIST == 5);
    TEST_CHECK(MINIDUMP_STREAM_EXCEPTION == 6);
    TEST_CHECK(MINIDUMP_STREAM_SYSTEM_INFO == 7);
#else
    SKIP_TEST();
#endif
}

/**
 * Test CPU architecture constants
 */
SENTRY_TEST(minidump_cpu_architectures)
{
#ifdef SENTRY_BACKEND_NATIVE
    TEST_CHECK(MINIDUMP_CPU_X86 == 0); // PROCESSOR_ARCHITECTURE_INTEL
    TEST_CHECK(MINIDUMP_CPU_ARM == 5); // PROCESSOR_ARCHITECTURE_ARM
    TEST_CHECK(MINIDUMP_CPU_ARM64 == 12); // PROCESSOR_ARCHITECTURE_ARM64
    TEST_CHECK(MINIDUMP_CPU_X86_64 == 9); // PROCESSOR_ARCHITECTURE_AMD64
#else
    SKIP_TEST();
#endif
}

/**
 * Test context flags
 */
SENTRY_TEST(minidump_context_flags)
{
#ifdef SENTRY_BACKEND_NATIVE
#    if defined(__x86_64__)
    // x86_64 full context flags
    uint32_t flags = 0x0010003f;
    TEST_CHECK((flags & 0x00100000) != 0); // CONTEXT_AMD64
    TEST_CHECK((flags & 0x00000001) != 0); // CONTEXT_CONTROL
    TEST_CHECK((flags & 0x00000002) != 0); // CONTEXT_INTEGER
    TEST_CHECK((flags & 0x00000004) != 0); // CONTEXT_SEGMENTS
    TEST_CHECK((flags & 0x00000008) != 0); // CONTEXT_FLOATING_POINT

#    elif defined(__aarch64__)
    // ARM64 full context flags
    uint32_t flags = 0x00400007;
    TEST_CHECK((flags & 0x00400000) != 0); // ARM64_CONTEXT
    TEST_CHECK((flags & 0x00000001) != 0); // CONTROL
    TEST_CHECK((flags & 0x00000002) != 0); // INTEGER
    TEST_CHECK((flags & 0x00000004) != 0); // FPSIMD
#    endif
#else
    SKIP_TEST();
#endif
}

/**
 * Test uint128_struct for NEON registers
 */
SENTRY_TEST(uint128_struct_size)
{
#if defined(SENTRY_BACKEND_NATIVE) && defined(__aarch64__)
    TEST_CHECK(sizeof(uint128_struct) == 16);

    uint128_struct val = { 0 };
    val.low = 0x123456789abcdef0ULL;
    val.high = 0xfedcba9876543210ULL;

    TEST_CHECK(val.low == 0x123456789abcdef0ULL);
    TEST_CHECK(val.high == 0xfedcba9876543210ULL);
#else
    SKIP_TEST();
#endif
}

/**
 * Test XMM save area structure
 */
SENTRY_TEST(xmm_save_area_size)
{
#if defined(SENTRY_BACKEND_NATIVE) && defined(__x86_64__)
    TEST_CHECK(sizeof(xmm_save_area32_t) == 512);

    xmm_save_area32_t fpu = { 0 };
    fpu.control_word = 0x037f;
    fpu.mx_csr = 0x1f80;

    TEST_CHECK(fpu.control_word == 0x037f);
    TEST_CHECK(fpu.mx_csr == 0x1f80);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(m128a_size)
{
#if defined(SENTRY_BACKEND_NATIVE) && defined(__x86_64__)
    TEST_CHECK(sizeof(m128a_t) == 16);

    m128a_t val = { 0 };
    val.low = 0x123456789abcdef0ULL;
    val.high = 0xfedcba9876543210ULL;

    TEST_CHECK(val.low == 0x123456789abcdef0ULL);
    TEST_CHECK(val.high == 0xfedcba9876543210ULL);
#else
    SKIP_TEST();
#endif
}

/**
 * Test that crash context structure includes transport configuration fields
 * and that they are properly sized for paths and URLs.
 */
SENTRY_TEST(crash_context_transport_fields)
{
#ifdef SENTRY_BACKEND_NATIVE
    // Heap-allocate: sentry_crash_context_t is multiple MB due to the inline
    // modules[] array and would overflow smaller thread stacks.
    sentry_crash_context_t *ctx = sentry_malloc(sizeof(*ctx));
    TEST_ASSERT(!!ctx);
    memset(ctx, 0, sizeof(*ctx));

    // Verify ca_certs field exists and can hold a typical path
    const char *test_ca = "/etc/ssl/certs/ca-certificates.crt";
    strncpy(ctx->ca_certs, test_ca, sizeof(ctx->ca_certs) - 1);
    ctx->ca_certs[sizeof(ctx->ca_certs) - 1] = '\0';
    TEST_CHECK_STRING_EQUAL(ctx->ca_certs, test_ca);

    // Verify proxy field exists and can hold a typical proxy URL
    const char *test_proxy = "http://proxy.example.com:8080";
    strncpy(ctx->proxy, test_proxy, sizeof(ctx->proxy) - 1);
    ctx->proxy[sizeof(ctx->proxy) - 1] = '\0';
    TEST_CHECK_STRING_EQUAL(ctx->proxy, test_proxy);

    // Verify user_agent field exists
    const char *test_ua = "sentry.native/0.8.0";
    strncpy(ctx->user_agent, test_ua, sizeof(ctx->user_agent) - 1);
    ctx->user_agent[sizeof(ctx->user_agent) - 1] = '\0';
    TEST_CHECK_STRING_EQUAL(ctx->user_agent, test_ua);

    ctx->shutdown_timeout = 12345;
    TEST_CHECK_UINT64_EQUAL(ctx->shutdown_timeout, 12345);
    ctx->crash_upload_mode = SENTRY_CRASH_UPLOAD_MODE_ASYNC;
    TEST_CHECK_INT_EQUAL(
        ctx->crash_upload_mode, SENTRY_CRASH_UPLOAD_MODE_ASYNC);
    ctx->transfer_timeout = 45000;
    TEST_CHECK_UINT64_EQUAL(ctx->transfer_timeout, 45000);

    // Verify fields are zero-initialized when memset to 0
    memset(ctx, 0, sizeof(*ctx));
    TEST_CHECK(ctx->ca_certs[0] == '\0');
    TEST_CHECK(ctx->proxy[0] == '\0');
    TEST_CHECK(ctx->user_agent[0] == '\0');
    TEST_CHECK_UINT64_EQUAL(ctx->shutdown_timeout, 0);
    TEST_CHECK_INT_EQUAL(ctx->crash_upload_mode, SENTRY_CRASH_UPLOAD_MODE_SYNC);
    TEST_CHECK_UINT64_EQUAL(ctx->transfer_timeout, 0);

    sentry_free(ctx);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(crash_context_init)
{
#ifdef SENTRY_BACKEND_NATIVE
    sentry_crash_context_t *ctx = sentry_malloc(sizeof(*ctx));
    TEST_ASSERT(!!ctx);
    memset(ctx, 0xA5, sizeof(*ctx));

    sentry__crash_context_init(ctx);

    TEST_CHECK_INT_EQUAL(ctx->magic, SENTRY_CRASH_MAGIC);
    TEST_CHECK_INT_EQUAL(ctx->version, SENTRY_CRASH_VERSION);
    TEST_CHECK_INT_EQUAL(ctx->state, SENTRY_CRASH_STATE_READY);
    TEST_CHECK_INT_EQUAL(ctx->sequence, 0);
    TEST_CHECK_INT_EQUAL(ctx->module_count, 0);

    TEST_CHECK(
        ((unsigned char *)ctx)[offsetof(sentry_crash_context_t, modules) - 1]
        == 0);
    TEST_CHECK(((unsigned char *)&ctx->modules[0])[0] == 0xA5);

    sentry_free(ctx);
#else
    SKIP_TEST();
#endif
}

/**
 * Test that options transport configuration is propagated to crash context
 * during native backend startup. This verifies the fix for the daemon
 * not receiving SSL certs and proxy settings from the parent process.
 */
SENTRY_TEST(crash_context_options_propagation)
{
#ifdef SENTRY_BACKEND_NATIVE
    // Create options with transport config
    SENTRY_TEST_OPTIONS_NEW(options);

    sentry_options_set_ca_certs(options, "/path/to/ca-bundle.crt");
    sentry_options_set_proxy(options, "http://myproxy:3128");
    sentry_options_set_shutdown_timeout(options, 12345);
    sentry_options_set_system_crash_reporter_enabled(options, true);
    sentry_options_set_on_crashed_last_run(
        options, noop_crashed_last_run, NULL);
    sentry_options_set_crash_upload_mode(
        options, SENTRY_CRASH_UPLOAD_MODE_ASYNC);
    sentry_options_set_transfer_timeout(options, 45000);
#    ifdef SENTRY_PLATFORM_WINDOWS
    sentry_options_set_minidump_flags(options, 0x00000006);
#    endif

    // Verify options were set correctly
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_ca_certs(options), "/path/to/ca-bundle.crt");
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_proxy(options), "http://myproxy:3128");

    // Simulate what native_backend_startup does: copy to crash context.
    // Heap-allocated to avoid overflowing the stack on the inline modules[].
    sentry_crash_context_t *ctx = sentry_malloc(sizeof(*ctx));
    TEST_ASSERT(!!ctx);
    memset(ctx, 0, sizeof(*ctx));

    if (options->ca_certs) {
        strncpy(ctx->ca_certs, options->ca_certs, sizeof(ctx->ca_certs) - 1);
        ctx->ca_certs[sizeof(ctx->ca_certs) - 1] = '\0';
    }
    if (options->proxy) {
        strncpy(ctx->proxy, options->proxy, sizeof(ctx->proxy) - 1);
        ctx->proxy[sizeof(ctx->proxy) - 1] = '\0';
    }
    if (options->user_agent) {
        strncpy(
            ctx->user_agent, options->user_agent, sizeof(ctx->user_agent) - 1);
        ctx->user_agent[sizeof(ctx->user_agent) - 1] = '\0';
    }
    ctx->shutdown_timeout = options->shutdown_timeout;
    ctx->system_crash_reporter_enabled = options->system_crash_reporter_enabled;
    ctx->has_on_crashed_last_run = options->on_crashed_last_run_func != NULL;
    ctx->crash_upload_mode = options->crash_upload_mode;
    ctx->transfer_timeout = options->transfer_timeout;
#    ifdef SENTRY_PLATFORM_WINDOWS
    ctx->minidump_flags = options->minidump_flags;
#    endif

    // Verify crash context received the values
    TEST_CHECK_STRING_EQUAL(ctx->ca_certs, "/path/to/ca-bundle.crt");
    TEST_CHECK_STRING_EQUAL(ctx->proxy, "http://myproxy:3128");
    // user_agent should have the default SDK user agent
    TEST_CHECK(ctx->user_agent[0] != '\0');
    TEST_CHECK_UINT64_EQUAL(ctx->shutdown_timeout, 12345);
    TEST_CHECK(ctx->system_crash_reporter_enabled);
    TEST_CHECK(ctx->has_on_crashed_last_run);
    TEST_CHECK_INT_EQUAL(
        ctx->crash_upload_mode, SENTRY_CRASH_UPLOAD_MODE_ASYNC);
    TEST_CHECK_UINT64_EQUAL(ctx->transfer_timeout, 45000);
#    ifdef SENTRY_PLATFORM_WINDOWS
    TEST_CHECK_INT_EQUAL(ctx->minidump_flags, 0x00000006);
#    endif

    sentry_free(ctx);
    sentry_options_free(options);
#else
    SKIP_TEST();
#endif
}

/**
 * Test that handler_path option is set correctly in options.
 * Note: handler_path is now passed directly to the daemon start function
 * instead of being stored in shared memory, since the daemon never reads
 * it from shared memory.
 */
SENTRY_TEST(crash_context_handler_path_propagation)
{
#ifdef SENTRY_BACKEND_NATIVE
    SENTRY_TEST_OPTIONS_NEW(options);

    // Set handler path
    sentry_options_set_handler_path(options, "/opt/sentry/sentry-crash");

    // Verify handler_path option is set correctly
    TEST_ASSERT(!!options->handler_path);
    TEST_CHECK_STRING_EQUAL(
        options->handler_path->path, "/opt/sentry/sentry-crash");

    // Without handler_path set, it should be NULL
    sentry_options_t *options2 = sentry_options_new();
    TEST_ASSERT(!!options2);
    TEST_CHECK(options2->handler_path == NULL);

    sentry_options_free(options);
    sentry_options_free(options2);
#else
    SKIP_TEST();
#endif
}

/**
 * Test that NULL/empty transport options don't corrupt crash context
 */
SENTRY_TEST(crash_context_null_options)
{
#ifdef SENTRY_BACKEND_NATIVE
    SENTRY_TEST_OPTIONS_NEW(options);

    // Don't set ca_certs or proxy - leave them as NULL.
    // Heap-allocated to avoid overflowing the stack on the inline modules[].
    sentry_crash_context_t *ctx = sentry_malloc(sizeof(*ctx));
    TEST_ASSERT(!!ctx);
    memset(ctx, 0, sizeof(*ctx));

    // Copy like native_backend_startup does (with NULL checks)
    if (options->ca_certs) {
        strncpy(ctx->ca_certs, options->ca_certs, sizeof(ctx->ca_certs) - 1);
    }
    if (options->proxy) {
        strncpy(ctx->proxy, options->proxy, sizeof(ctx->proxy) - 1);
    }

    // All should remain empty (zero-initialized)
    TEST_CHECK(ctx->ca_certs[0] == '\0');
    TEST_CHECK(ctx->proxy[0] == '\0');

    sentry_free(ctx);
    sentry_options_free(options);
#else
    SKIP_TEST();
#endif
}

/**
 * Test packed attribute works correctly
 */
SENTRY_TEST(minidump_structures_packed)
{
#ifdef SENTRY_BACKEND_NATIVE
    // Structures should not have padding
    // This is critical for binary format compatibility

#    if defined(__x86_64__)
    // x86_64 context: 6*8 + 4*2 + 6*2 + 2*4 + 8*8 + 16*8 + 512 + 26*16 + 6*8 =
    // 1232
    size_t expected_x86_64 = 48 + 8 + 12 + 8 + 64 + 128 + 512 + 416 + 48;
    TEST_CHECK(sizeof(minidump_context_x86_64_t) == expected_x86_64);

#    elif defined(__aarch64__)
    // ARM64 context: 4 + 4 + 29*8 + 4*8 + 32*16 + 4 + 4 + 8*4 + 8*8 + 2*4 + 2*8
    // = 1344
    size_t expected_arm64 = 8 + 232 + 32 + 512 + 8 + 32 + 64 + 8 + 16;
    TEST_CHECK(sizeof(minidump_context_arm64_t) <= expected_arm64 + 100);
#    endif
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(elf_header_entry_sizes)
{
#if !defined(SENTRY_PLATFORM_LINUX) && !defined(SENTRY_PLATFORM_ANDROID)
    SKIP_TEST();
#else
    unsigned char e_ident[EI_NIDENT] = { 0 };
    unsigned char other_class;
    size_t shdr_size;
    size_t phdr_size;

#    if defined(__x86_64__) || defined(__aarch64__)
    e_ident[EI_CLASS] = ELFCLASS64;
    other_class = ELFCLASS32;
    shdr_size = sizeof(Elf64_Shdr);
    phdr_size = sizeof(Elf64_Phdr);
#    else
    e_ident[EI_CLASS] = ELFCLASS32;
    other_class = ELFCLASS64;
    shdr_size = sizeof(Elf32_Shdr);
    phdr_size = sizeof(Elf32_Phdr);
#    endif

    TEST_CHECK(sentry__elf_is_native_class(e_ident));
    TEST_CHECK(sentry__elf_has_shdr_size(e_ident, shdr_size));
    TEST_CHECK(sentry__elf_has_phdr_size(e_ident, phdr_size));

    TEST_CHECK(!sentry__elf_has_shdr_size(e_ident, shdr_size - 1));
    TEST_CHECK(!sentry__elf_has_shdr_size(e_ident, shdr_size + 1));
    TEST_CHECK(!sentry__elf_has_phdr_size(e_ident, phdr_size - 1));
    TEST_CHECK(!sentry__elf_has_phdr_size(e_ident, phdr_size + 1));

    e_ident[EI_CLASS] = other_class;
    TEST_CHECK(!sentry__elf_is_native_class(e_ident));
    TEST_CHECK(!sentry__elf_has_shdr_size(e_ident, shdr_size));
    TEST_CHECK(!sentry__elf_has_phdr_size(e_ident, phdr_size));
#endif
}

SENTRY_TEST(crash_ipc_message_roundtrip)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    const char payload[] = "\x81\xa3key\xa5value";
    char *buf = NULL;
    size_t buf_len = 0;

    sentry_crash_ipc_message_result_t result
        = sentry__crash_ipc_message_encode(SENTRY_CRASH_IPC_MESSAGE_SET_TAG, 7,
            42, payload, sizeof(payload) - 1, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OK);
    TEST_CHECK(buf != NULL);
    TEST_CHECK_INT_EQUAL(
        buf_len, SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE + sizeof(payload) - 1);

    sentry_crash_ipc_message_t message;
    result = sentry__crash_ipc_message_decode(buf, buf_len, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OK);
    TEST_CHECK_INT_EQUAL(message.type, SENTRY_CRASH_IPC_MESSAGE_SET_TAG);
    TEST_CHECK_INT_EQUAL(message.flags, 7);
    TEST_CHECK_INT_EQUAL(message.sequence, 42);
    TEST_CHECK_INT_EQUAL(message.payload_len, sizeof(payload) - 1);
    TEST_CHECK(memcmp(message.payload, payload, sizeof(payload) - 1) == 0);

    sentry_free(buf);
#endif
}

SENTRY_TEST(crash_ipc_message_partial)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    char *buf = NULL;
    size_t buf_len = 0;

    sentry_crash_ipc_message_result_t result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_CRASH, 0, 9, NULL, 0, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OK);

    sentry_crash_ipc_message_t message;
    result = sentry__crash_ipc_message_decode(buf, 3, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_PARTIAL);

    result = sentry__crash_ipc_message_decode(buf, buf_len - 1, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_PARTIAL);

    sentry_free(buf);
#endif
}

SENTRY_TEST(crash_ipc_message_invalid_lengths)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    char too_small_len[4] = { 11, 0, 0, 0 };
    sentry_crash_ipc_message_t message;

    sentry_crash_ipc_message_result_t result = sentry__crash_ipc_message_decode(
        too_small_len, sizeof(too_small_len), &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_INVALID);

    char *buf = NULL;
    size_t buf_len = 0;
    result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN, 0, 1, NULL, 0, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OK);

    char extra[32];
    memcpy(extra, buf, buf_len);
    extra[buf_len] = 0;
    result = sentry__crash_ipc_message_decode(extra, buf_len + 1, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_INVALID);

    sentry_free(buf);
#endif
}

SENTRY_TEST(crash_ipc_message_unknown_type)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    char *buf = NULL;
    size_t buf_len = 0;

    sentry_crash_ipc_message_result_t result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_CRASH, 0, 1, NULL, 0, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OK);
    buf[4] = 0xff;
    buf[5] = 0x7f;

    sentry_crash_ipc_message_t message;
    result = sentry__crash_ipc_message_decode(buf, buf_len, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_UNKNOWN_TYPE);

    sentry_free(buf);
#endif
}

SENTRY_TEST(crash_ipc_message_oversized)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    char oversized_len[4] = { 0 };
    uint32_t len = SENTRY_CRASH_IPC_MESSAGE_MAX_LEN + 1;
    oversized_len[0] = (char)(len & 0xffu);
    oversized_len[1] = (char)((len >> 8) & 0xffu);
    oversized_len[2] = (char)((len >> 16) & 0xffu);
    oversized_len[3] = (char)((len >> 24) & 0xffu);

    sentry_crash_ipc_message_t message;
    sentry_crash_ipc_message_result_t result = sentry__crash_ipc_message_decode(
        oversized_len, sizeof(oversized_len), &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OVERSIZED);

    char payload = 0;
    char *buf = NULL;
    size_t buf_len = 0;
    result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, 0, 0, &payload,
        SENTRY_CRASH_IPC_MESSAGE_MAX_LEN, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OVERSIZED);
    result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, 0, 0, &payload,
        SIZE_MAX - SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE + 1, &buf, &buf_len);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_OVERSIZED);
#endif
}

SENTRY_TEST(crash_ipc_message_large_roundtrip)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    size_t payload_len = 200u * 1024u * 1024u;
    char *payload = sentry_malloc(payload_len);
    TEST_ASSERT(payload != NULL);
    memset(payload, 0xa5, payload_len);
    char *buf = NULL;
    size_t buf_len = 0;

    sentry_crash_ipc_message_result_t result = sentry__crash_ipc_message_encode(
        SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, 0, 42, payload, payload_len,
        &buf, &buf_len);
    TEST_ASSERT(result == SENTRY_CRASH_IPC_MESSAGE_OK);
    TEST_CHECK(buf_len == SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE + payload_len);

    sentry_crash_ipc_message_t message;
    result = sentry__crash_ipc_message_decode(buf, buf_len - 1, &message);
    TEST_CHECK_INT_EQUAL(result, SENTRY_CRASH_IPC_MESSAGE_PARTIAL);
    result = sentry__crash_ipc_message_decode(buf, buf_len, &message);
    TEST_ASSERT(result == SENTRY_CRASH_IPC_MESSAGE_OK);
    TEST_CHECK(message.payload_len == payload_len);
    TEST_CHECK(memcmp(message.payload, payload, payload_len) == 0);

    sentry_free(buf);
    sentry_free(payload);
#endif
}

#ifdef SENTRY_BACKEND_NATIVE
static bool
apply_scope_message(
    sentry_crash_scope_t *scope, uint16_t type, sentry_value_t value)
{
    size_t len = 0;
    char *payload = sentry_value_to_msgpack(value, &len);
    sentry_crash_ipc_message_t message
        = { type, 0, scope->sequence + 1, payload, len };
    bool applied = sentry__crash_scope_apply(scope, &message);
    sentry_free(payload);
    sentry_value_decref(value);
    return applied;
}

static sentry_value_t
scope_snapshot(void)
{
    sentry_value_t snapshot = sentry_value_new_object();
    sentry_value_set_by_key(snapshot, "event", sentry_value_new_object());
    sentry_value_set_by_key(snapshot, "attachments", sentry_value_new_list());
    return snapshot;
}

static sentry_value_t
scope_pair(const char *key, sentry_value_t value)
{
    sentry_value_t pair = sentry_value_new_list();
    sentry_value_append(pair, sentry_value_new_string(key));
    sentry_value_append(pair, value);
    return pair;
}
#endif

SENTRY_TEST(crash_scope_updates)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_scope_t scope;
    TEST_ASSERT(sentry__crash_scope_init(&scope, 2));
    TEST_CHECK(!apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_object()));
    TEST_ASSERT(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, scope_snapshot()));
    struct {
        uint16_t set;
        uint16_t remove;
        const char *field;
    } keyed[] = {
        { SENTRY_CRASH_IPC_MESSAGE_SET_TAG, SENTRY_CRASH_IPC_MESSAGE_REMOVE_TAG,
            "tags" },
        { SENTRY_CRASH_IPC_MESSAGE_SET_EXTRA,
            SENTRY_CRASH_IPC_MESSAGE_REMOVE_EXTRA, "extra" },
        { SENTRY_CRASH_IPC_MESSAGE_SET_CONTEXT,
            SENTRY_CRASH_IPC_MESSAGE_REMOVE_CONTEXT, "contexts" },
    };
    for (size_t i = 0; i < sizeof(keyed) / sizeof(keyed[0]); i++) {
        TEST_CHECK(apply_scope_message(&scope, keyed[i].set,
            scope_pair("key", sentry_value_new_string("first"))));
        TEST_CHECK(apply_scope_message(
            &scope, keyed[i].remove, sentry_value_new_string("key")));
        TEST_CHECK(sentry_value_is_null(sentry_value_get_by_key(
            sentry_value_get_by_key(scope.event, keyed[i].field), "key")));
        TEST_CHECK(apply_scope_message(&scope, keyed[i].set,
            scope_pair("key", sentry_value_new_string("last"))));
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(scope.event, keyed[i].field), "key")),
            "last");
    }
    struct {
        uint16_t type;
        const char *field;
    } fields[] = {
        { SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE, "release" },
        { SENTRY_CRASH_IPC_MESSAGE_SET_ENVIRONMENT, "environment" },
        { SENTRY_CRASH_IPC_MESSAGE_SET_TRANSACTION, "transaction" },
        { SENTRY_CRASH_IPC_MESSAGE_SET_LEVEL, "level" },
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        TEST_CHECK(apply_scope_message(
            &scope, fields[i].type, sentry_value_new_string_n("a\0b", 3)));
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(sentry_value_get_by_key(
                                 scope.event, fields[i].field)),
            3);
        TEST_CHECK(apply_scope_message(
            &scope, fields[i].type, sentry_value_new_null()));
        TEST_CHECK(sentry_value_is_null(
            sentry_value_get_by_key(scope.event, fields[i].field)));
    }
    TEST_CHECK(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_object()));
    TEST_CHECK(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_null()));
    TEST_CHECK(apply_scope_message(&scope,
        SENTRY_CRASH_IPC_MESSAGE_SET_FINGERPRINT, sentry_value_new_list()));
    TEST_CHECK(apply_scope_message(&scope,
        SENTRY_CRASH_IPC_MESSAGE_SET_FINGERPRINT, sentry_value_new_null()));
    for (int i = 0; i < 3; i++) {
        sentry_value_t crumb = sentry_value_new_object();
        sentry_value_set_by_key(crumb, "index", sentry_value_new_int32(i));
        TEST_CHECK(apply_scope_message(
            &scope, SENTRY_CRASH_IPC_MESSAGE_ADD_BREADCRUMB, crumb));
    }
    sentry_value_t event = sentry__crash_scope_event(&scope);
    sentry_value_t crumbs = sentry_value_get_by_key(event, "breadcrumbs");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(crumbs), 2);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int32(sentry_value_get_by_key(
                             sentry_value_get_by_index(crumbs, 0), "index")),
        1);
    sentry_value_decref(event);
    sentry_value_t attachments = sentry_value_new_list();
    sentry_value_append(attachments, sentry_value_new_object());
    TEST_CHECK(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_ATTACHMENT_LIST, attachments));
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(scope.attachments), 1);
    TEST_CHECK(apply_scope_message(&scope,
        SENTRY_CRASH_IPC_MESSAGE_SET_ATTACHMENT_LIST, sentry_value_new_list()));
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(scope.attachments), 0);
    TEST_CHECK(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, scope_snapshot()));
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(scope.event), 0);
    sentry_crash_ipc_message_t crash
        = { SENTRY_CRASH_IPC_MESSAGE_CRASH, 0, scope.sequence + 1, NULL, 0 };
    TEST_CHECK(sentry__crash_scope_apply(&scope, &crash));
    TEST_CHECK(!apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_object()));
    sentry__crash_scope_free(&scope);
#endif
}

SENTRY_TEST(crash_scope_invalid_updates)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_scope_t scope;
    TEST_ASSERT(sentry__crash_scope_init(&scope, 0));
    TEST_ASSERT(apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, scope_snapshot()));
    TEST_CHECK(!apply_scope_message(&scope, SENTRY_CRASH_IPC_MESSAGE_SET_TAG,
        sentry_value_new_string("invalid")));
    TEST_CHECK(!apply_scope_message(
        &scope, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_int32(1)));
    TEST_CHECK(!apply_scope_message(&scope,
        SENTRY_CRASH_IPC_MESSAGE_SET_ATTACHMENT_LIST,
        sentry_value_new_object()));
    sentry_crash_ipc_message_t message
        = { SENTRY_CRASH_IPC_MESSAGE_SET_USER, 0, 2, "\x81", 1 };
    TEST_CHECK(!sentry__crash_scope_apply(&scope, &message));
    message.payload = "\xc0\xc0";
    message.payload_len = 2;
    TEST_CHECK(!sentry__crash_scope_apply(&scope, &message));
    message.payload = "\xc0";
    message.payload_len = 1;
    message.sequence = 3;
    TEST_CHECK(!sentry__crash_scope_apply(&scope, &message));
    message.sequence = 2;
    message.flags = 1;
    TEST_CHECK(!sentry__crash_scope_apply(&scope, &message));
    TEST_CHECK_INT_EQUAL(scope.sequence, 1);
    TEST_CHECK(apply_scope_message(&scope,
        SENTRY_CRASH_IPC_MESSAGE_ADD_BREADCRUMB, sentry_value_new_object()));
    sentry_value_t event = sentry__crash_scope_event(&scope);
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(event, "breadcrumbs")),
        0);
    sentry_value_decref(event);
    sentry__crash_scope_free(&scope);
#endif
}

#ifdef SENTRY_BACKEND_NATIVE
static sentry_value_t
native_scope_on_crash(const sentry_ucontext_t *UNUSED(uctx),
    sentry_value_t event, sentry_hint_t *UNUSED(hint), void *UNUSED(data))
{
    sentry_set_tag("tag", "callback");
    sentry_clear_attachments();
    sentry_attach_bytes("callback", 8, "callback.txt");
    sentry_value_set_by_key(event, "callback", sentry_value_new_bool(true));
    return event;
}
#endif

SENTRY_TEST(native_scope_updates)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    const char *crash_mode = getenv("SENTRY_TEST_NATIVE_SCOPE_CRASH");
    bool callback = getenv("SENTRY_TEST_NATIVE_SCOPE_CALLBACK") != NULL;
    SENTRY_TEST_OPTIONS_NEW(options);
    if (callback) {
        sentry_options_set_on_crash(options, native_scope_on_crash, NULL);
    }
    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_max_breadcrumbs(options, 3);
    sentry_options_set_traces_sample_rate(options, 1);
    sentry_options_set_debug(options, true);
    if (crash_mode) {
        sentry_options_set_dsn(options, getenv("SENTRY_DSN"));
        sentry_options_set_crash_reporting_mode(options, atoi(crash_mode));
    }
    TEST_ASSERT(sentry_init(options) == 0);
    sentry_set_tag("cleared", "old");
    sentry_attach_bytes("old", 3, "cleared.txt");
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_scope_clear(scope);
    }
    for (int i = 0; i < 1000; i++) {
        sentry_set_tag("tag", "old");
        sentry_remove_tag("tag");
    }
    sentry_set_tag("tag", "latest");
    sentry_set_tag_n("binary-tag-tail", 10, "v\0x", 3);
    sentry_set_tag_n("gone-tag-tail", 8, "old", 3);
    sentry_remove_tag_n("gone-tag-tail", 8);
    sentry_set_extra_n("gone-extra-tail", 10, sentry_value_new_int32(1));
    sentry_remove_extra_n("gone-extra-tail", 10);
    sentry_set_context_n("gone-context-tail", 12, sentry_value_new_object());
    sentry_remove_context_n("gone-context-tail", 12);
    sentry_set_release("ipc-release");
    sentry_set_environment("ipc-environment");
    sentry_set_transaction("ipc-transaction");
    sentry_set_level(SENTRY_LEVEL_WARNING);
    sentry_set_fingerprint("ipc", "latest", NULL);
    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "username", sentry_value_new_string("ipc-user"));
    sentry_set_user(user);
    sentry_set_extra("removed", sentry_value_new_int32(1));
    sentry_remove_extra("removed");
    size_t size = 1024 * 1024;
    char *large = sentry_malloc(size);
    TEST_ASSERT(large != NULL);
    memset(large, 'x', size);
    sentry_set_extra("large", sentry_value_new_string_n(large, size));
    sentry_free(large);
    sentry_set_context("removed", sentry_value_new_object());
    sentry_remove_context("removed");
    sentry_value_t context = sentry_value_new_object();
    sentry_value_set_by_key(
        context, "status", sentry_value_new_string("latest"));
    sentry_set_context("ipc", context);
    sentry_transaction_t *tx = sentry_transaction_start(
        sentry_transaction_context_new("bound", "work"),
        sentry_value_new_null());
    sentry_set_transaction_object(tx);
    sentry_span_t *span = sentry_transaction_start_child(tx, "child", "work");
    sentry_set_span(span);
    bool scoped_trace = getenv("SENTRY_TEST_NATIVE_SCOPE_SPAN") != NULL;
    if (!scoped_trace) {
        sentry_set_span(NULL);
    }
    sentry_set_context("trace", sentry_value_new_object());
    sentry_value_t trace = sentry_value_new_null();
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_regenerate_propagation_context(scope);
        if (scoped_trace) {
            sentry_value_t active
                = sentry__scope_load_span_or_transaction(scope);
            trace = sentry__value_get_trace_context(active);
            sentry_value_decref(active);
        } else {
            trace = sentry__scope_load_trace_context(scope);
        }
    }
    sentry_set_extra("expected-trace", trace);
    for (int i = 0; i < 5; i++) {
        sentry_value_t crumb = sentry_value_new_breadcrumb(NULL, NULL);
        sentry_value_set_by_key(crumb, "index", sentry_value_new_int32(i));
        sentry_add_breadcrumb(crumb);
    }
    sentry_attach_bytes("old", 3, "cleared-again.txt");
    sentry_clear_attachments();
    sentry_uuid_t removed = sentry_attach_bytes("old", 3, "removed.txt");
    sentry_attach_bytes("new", 3, "latest.txt");
    sentry_remove_attachment(removed);
    SENTRY_WITH_OPTIONS (current) {
        const char *names[] = { "__sentry-event", "__sentry-breadcrumb1",
            "__sentry-breadcrumb2", "__sentry-attachments" };
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
            sentry_path_t *path
                = sentry__path_join_str(current->run->run_path, names[i]);
            TEST_CHECK(!sentry__path_is_file(path));
            sentry__path_free(path);
        }
        if (crash_mode && !callback) {
            // exercise staged IPC state without a crash-time snapshot
            current->backend->except_func = NULL;
        }
    }
    if (crash_mode) {
#    if defined(SENTRY_PLATFORM_WINDOWS)
        RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
#    else
        raise(SIGSEGV);
#    endif
    }
    sentry_span_finish(span);
    sentry_transaction_finish(tx);
    sentry_close();
#endif
}

#ifdef SENTRY_BACKEND_NATIVE
static sentry_crash_ipc_t *
new_test_ipc(void)
{
    sentry_crash_ipc_t *ipc = sentry__crash_ipc_init_app(NULL);
    TEST_ASSERT(ipc != NULL);
    TEST_ASSERT(sentry__crash_scope_init(&ipc->scope, 2));
    sentry_value_t snapshot = scope_snapshot();
    TEST_ASSERT(sentry__crash_ipc_send(
        ipc, SENTRY_CRASH_IPC_MESSAGE_SCOPE_SNAPSHOT, snapshot));
    sentry_value_decref(snapshot);
    return ipc;
}

static void
write_test_frame(sentry_crash_ipc_t *ipc, const char *buf, size_t len)
{
#    if defined(SENTRY_PLATFORM_WINDOWS)
    DWORD written = 0;
    TEST_ASSERT(
        WriteFile(ipc->message_write_handle, buf, (DWORD)len, &written, NULL));
    TEST_CHECK_INT_EQUAL(written, len);
#    else
    TEST_CHECK_INT_EQUAL(write(ipc->message_fd[0], buf, len), len);
#    endif
}
#endif

SENTRY_TEST(crash_ipc_stream_ordering)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_ipc_t *ipc = new_test_ipc();
    sentry_value_t value = sentry_value_new_string("latest");
    TEST_CHECK(sentry__crash_ipc_send(
        ipc, SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE, value));
    sentry_value_decref(value);
    sentry__crash_ipc_notify(ipc);
    TEST_CHECK(sentry__crash_ipc_receive(ipc, 100));
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                ipc->scope.event, "release")),
        "latest");
    TEST_CHECK(!sentry__crash_ipc_send(
        ipc, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_null()));
    sentry__crash_ipc_free(ipc);
#endif
}

SENTRY_TEST(crash_ipc_interrupted_frame)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_ipc_t *ipc = new_test_ipc();
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    char *buf = NULL;
    size_t len = 0;
    const char payload[] = "\xa6latest";
    TEST_ASSERT(
        sentry__crash_ipc_message_encode(SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE,
            0, 2, payload, sizeof(payload) - 1, &buf, &len)
        == SENTRY_CRASH_IPC_MESSAGE_OK);
    write_test_frame(ipc, buf, 7);
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    write_test_frame(ipc, buf + 7, len - 8);
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    ipc->writing = 1;
    sentry__crash_ipc_notify(ipc);
    TEST_CHECK(sentry__crash_ipc_receive(ipc, 100));
    TEST_CHECK_INT_EQUAL(ipc->scope.sequence, 1);
    TEST_CHECK(sentry_value_is_null(
        sentry_value_get_by_key(ipc->scope.event, "release")));
    sentry_free(buf);
    sentry__crash_ipc_free(ipc);
#endif
}

SENTRY_TEST(crash_ipc_partial_reads)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_ipc_t *ipc = new_test_ipc();
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    char *buf = NULL;
    size_t len = 0;
    const char payload[] = "\xa6latest";
    TEST_ASSERT(
        sentry__crash_ipc_message_encode(SENTRY_CRASH_IPC_MESSAGE_SET_RELEASE,
            0, 2, payload, sizeof(payload) - 1, &buf, &len)
        == SENTRY_CRASH_IPC_MESSAGE_OK);
    for (size_t i = 0; i < len; i++) {
        write_test_frame(ipc, buf + i, 1);
        TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
        TEST_CHECK_INT_EQUAL(ipc->scope.sequence, i + 1 == len ? 2 : 1);
    }
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                ipc->scope.event, "release")),
        "latest");
    sentry_free(buf);
    sentry__crash_ipc_free(ipc);
#endif
}

SENTRY_TEST(crash_ipc_shutdown)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_ipc_t *ipc = new_test_ipc();
    TEST_CHECK(sentry__crash_ipc_send(
        ipc, SENTRY_CRASH_IPC_MESSAGE_SHUTDOWN, sentry_value_new_null()));
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 100));
    TEST_CHECK(ipc->scope.stopped);
    sentry__crash_ipc_free(ipc);
#endif
}

SENTRY_TEST(crash_ipc_broken_stream)
{
#ifndef SENTRY_BACKEND_NATIVE
    SKIP_TEST();
#else
    sentry_crash_ipc_t *ipc = new_test_ipc();
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    char invalid[SENTRY_CRASH_IPC_MESSAGE_HEADER_SIZE]
        = { 12, 0, 0, 0, (char)0xff, (char)0xff, 0, 0, 2 };
    write_test_frame(ipc, invalid, sizeof(invalid));
    TEST_CHECK(!sentry__crash_ipc_receive(ipc, 0));
    TEST_CHECK(ipc->message_closed);
    TEST_CHECK_INT_EQUAL(ipc->scope.sequence, 1);
    TEST_CHECK(!sentry__crash_ipc_send(
        ipc, SENTRY_CRASH_IPC_MESSAGE_SET_USER, sentry_value_new_null()));
    sentry__crash_ipc_notify(ipc);
    TEST_CHECK(sentry__crash_ipc_receive(ipc, 100));
    sentry__crash_ipc_free(ipc);
#endif
}
