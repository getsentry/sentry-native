#ifndef SENTRY_MINIDUMP_COMMON_H_INCLUDED
#define SENTRY_MINIDUMP_COMMON_H_INCLUDED

#include "sentry_minidump_format.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * Common minidump writer base structure
 * Platform-specific writers embed this as their first member
 */
typedef struct {
    int fd;
    uint64_t current_offset;
} minidump_writer_base_t;

/**
 * Write data to minidump file and return RVA
 * Automatically aligns to 4-byte boundary
 *
 * @param writer Pointer to writer base (or struct with base as first member)
 * @param data Data to write
 * @param size Size of data
 * @return RVA of written data, or 0 on failure
 */
minidump_rva_t sentry__minidump_write_data(
    minidump_writer_base_t *writer, const void *data, size_t size);

/**
 * Write minidump header
 *
 * @param writer Pointer to writer base
 * @param stream_count Number of streams in the minidump
 * @return 0 on success, -1 on failure
 */
int sentry__minidump_write_header(
    minidump_writer_base_t *writer, uint32_t stream_count);

/**
 * Write UTF-16LE string for minidump
 * Converts UTF-8 string to UTF-16LE with length prefix
 *
 * @param writer Pointer to writer base
 * @param utf8_str UTF-8 string to convert and write
 * @return RVA of written string, or 0 on failure
 */
minidump_rva_t sentry__minidump_write_string(
    minidump_writer_base_t *writer, const char *utf8_str);

/**
 * Get size of thread context for current architecture
 *
 * @return Size of the architecture-specific context structure
 */
size_t sentry__minidump_get_context_size(void);

/**
 * Read memory from the crashed process
 *
 * @param writer Pointer to writer base
 * @param addr Address to read from
 * @param buf Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or a negative value on failure
 */
typedef ssize_t (*sentry_minidump_read_fn_t)(
    minidump_writer_base_t *writer, uint64_t addr, void *buf, size_t size);

/**
 * Write a Memory64List stream with the memory of the given regions
 *
 * Must be the last stream, because the memory can extend past the 4GB limit
 * of 32-bit RVAs.
 *
 * On return, `list` describes the captured memory. A region ends at its first
 * unreadable byte, and regions with no readable bytes are dropped.
 *
 * @param writer Pointer to writer base
 * @param dir Directory entry to fill in
 * @param list Regions to capture
 * @param read_memory Function to read the memory with
 * @return 0 on success, -1 on failure
 */
int sentry__minidump_write_memory64_list(minidump_writer_base_t *writer,
    minidump_directory_t *dir, minidump_memory64_list_t *list,
    sentry_minidump_read_fn_t read_memory);

/**
 * Add a range of a region to the list, growing the list as needed
 *
 * Extends the last range if it is contiguous and in the same region. Regions
 * stay separate, because a range ends at its first unreadable byte.
 *
 * @param list List to add to, replaced when it grows
 * @param capacity Number of ranges the list can hold
 * @param region Start address of the region
 * @param start Start address of the range
 * @param size Size of the range
 * @return true on success, false on allocation failure
 */
bool sentry__minidump_add_range(minidump_memory64_list_t **list,
    uint64_t *capacity, uint64_t region, uint64_t start, uint64_t size);

#endif // SENTRY_MINIDUMP_COMMON_H_INCLUDED
