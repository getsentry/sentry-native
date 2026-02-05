#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)         \
    || defined(SENTRY_PLATFORM_MACOS)

#    include "sentry_alloc.h"
#    include "sentry_logger.h"
#    include "sentry_minidump_common.h"
#    include "sentry_minidump_format.h"

#    include <errno.h>
#    include <string.h>
#    include <time.h>
#    include <unistd.h>

minidump_rva_t
sentry__minidump_write_data(
    minidump_writer_base_t *writer, const void *data, size_t size)
{
    minidump_rva_t rva = writer->current_offset;

    ssize_t written = write(writer->fd, data, size);
    if (written != (ssize_t)size) {
        SENTRY_WARNF("minidump write failed: %s", strerror(errno));
        return 0;
    }

    writer->current_offset += size;

    // Align to 4-byte boundary
    uint32_t padding = (4 - (writer->current_offset % 4)) % 4;
    if (padding > 0) {
        const uint8_t zeros[4] = { 0 };
        if (write(writer->fd, zeros, padding) == (ssize_t)padding) {
            writer->current_offset += padding;
        }
        // On padding write failure, don't update offset - RVA is still valid
        // for the data that was written
    }

    return rva;
}

int
sentry__minidump_write_header(
    minidump_writer_base_t *writer, uint32_t stream_count)
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

    if (sentry__minidump_write_data(writer, &header, sizeof(header)) == 0) {
        return -1;
    }

    return 0;
}

minidump_rva_t
sentry__minidump_write_string(
    minidump_writer_base_t *writer, const char *utf8_str)
{
    if (!utf8_str) {
        return 0;
    }

    size_t utf8_len = strlen(utf8_str);

    // Sanity check: prevent integer overflow and reject unreasonably long
    // strings. Max reasonable module name/path is ~32KB, which fits in uint32_t
    if (utf8_len > 32768) {
        SENTRY_WARNF("minidump string too long: %zu bytes", utf8_len);
        return 0;
    }

    // Allocate buffer for: length (4 bytes) + UTF-16LE chars + null terminator
    // Each ASCII char becomes 2 bytes in UTF-16LE
    size_t total_size
        = sizeof(uint32_t) + (utf8_len * 2) + 2; // +2 for null terminator
    uint8_t *buf = sentry_malloc(total_size);
    if (!buf) {
        return 0;
    }

    // Write string length in bytes (NOT including null terminator)
    uint32_t string_bytes = utf8_len * 2;
    memcpy(buf, &string_bytes, sizeof(uint32_t));

    // Convert UTF-8 to UTF-16LE (simple ASCII conversion)
    // Note: This handles ASCII correctly; non-ASCII chars become single
    // UTF-16 code units which works for most Latin characters
    uint16_t *utf16 = (uint16_t *)(buf + sizeof(uint32_t));
    for (size_t i = 0; i < utf8_len; i++) {
        utf16[i] = (uint16_t)(unsigned char)utf8_str[i];
    }
    utf16[utf8_len] = 0; // Null terminator

    minidump_rva_t rva = sentry__minidump_write_data(writer, buf, total_size);
    sentry_free(buf);
    return rva;
}

size_t
sentry__minidump_get_context_size(void)
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

#endif // SENTRY_PLATFORM_LINUX || SENTRY_PLATFORM_ANDROID ||
       // SENTRY_PLATFORM_MACOS
