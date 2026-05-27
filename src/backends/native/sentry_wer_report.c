#include "sentry_wer_report.h"

#include "sentry_alloc.h"
#include "sentry_string.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

static bool
format_wstring(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    int written = _vsnwprintf(buffer, size, format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= size) {
        buffer[0] = L'\0';
        return false;
    }
    return true;
}

static bool
read_file(const wchar_t *path, DWORD max_size, char **buffer, DWORD *read)
{
    *buffer = NULL;
    *read = 0;

    HANDLE file = CreateFileW(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD size = GetFileSize(file, NULL);
    if (size == INVALID_FILE_SIZE || size > max_size) {
        CloseHandle(file);
        return false;
    }

    *buffer = (char *)sentry_malloc((size_t)size);
    if (!*buffer) {
        CloseHandle(file);
        return false;
    }

    bool success = ReadFile(file, *buffer, size, read, NULL);
    CloseHandle(file);
    if (!success) {
        sentry_free(*buffer);
        *buffer = NULL;
        *read = 0;
    }
    return success;
}

static bool
decode_buffer(
    const char *buffer, size_t size, wchar_t **decoded, const wchar_t **text)
{
    *decoded = NULL;
    *text = NULL;

    if (!buffer || size > INT_MAX) {
        return false;
    }

    if (size >= 2 && (unsigned char)buffer[0] == 0xff
        && (unsigned char)buffer[1] == 0xfe) {
        size_t wchar_count = (size - 2) / sizeof(wchar_t);
        *decoded
            = (wchar_t *)sentry_malloc(sizeof(wchar_t) * (wchar_count + 1));
        if (!*decoded) {
            return false;
        }
        memcpy(*decoded, buffer + 2, wchar_count * sizeof(wchar_t));
        (*decoded)[wchar_count] = L'\0';
        *text = *decoded;
        return true;
    }

    int len = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, buffer, (int)size, NULL, 0);
    UINT codepage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (len <= 0) {
        codepage = CP_ACP;
        flags = 0;
        len = MultiByteToWideChar(codepage, flags, buffer, (int)size, NULL, 0);
    }
    if (len <= 0) {
        return false;
    }

    *decoded = (wchar_t *)sentry_malloc(sizeof(wchar_t) * ((size_t)len + 1));
    if (!*decoded) {
        return false;
    }

    int written = MultiByteToWideChar(
        codepage, flags, buffer, (int)size, *decoded, len);
    if (written != len) {
        sentry_free(*decoded);
        *decoded = NULL;
        return false;
    }
    (*decoded)[len] = L'\0';
    *text = *decoded;
    return true;
}

static const wchar_t *
find_wchar_before(const wchar_t *start, const wchar_t *end, wchar_t ch)
{
    while (start < end) {
        if (*start == ch) {
            return start;
        }
        start++;
    }
    return NULL;
}

static const wchar_t *
wcsstr_before(
    const wchar_t *haystack, const wchar_t *needle, const wchar_t *end)
{
    const wchar_t *found = wcsstr(haystack, needle);
    size_t needle_len = wcslen(needle);
    return found && (!end || found + needle_len <= end) ? found : NULL;
}

static bool
find_xml_section(const wchar_t *xml, const wchar_t *tag,
    const wchar_t **content_start, const wchar_t **content_end)
{
    wchar_t open_tag[64];
    wchar_t close_tag[64];
    if (!format_wstring(
            open_tag, sizeof(open_tag) / sizeof(wchar_t), L"<%s>", tag)
        || !format_wstring(
            close_tag, sizeof(close_tag) / sizeof(wchar_t), L"</%s>", tag)) {
        return false;
    }

    const wchar_t *open = wcsstr(xml, open_tag);
    if (!open) {
        return false;
    }
    *content_start = open + wcslen(open_tag);

    const wchar_t *close = wcsstr(*content_start, close_tag);
    if (!close) {
        return false;
    }

    *content_end = close;
    return true;
}

static bool
copy_xml_tag(const wchar_t *xml, const wchar_t *tag, wchar_t *out, size_t len)
{
    if (!xml || !tag || !out || !len) {
        return false;
    }

    out[0] = L'\0';

    const wchar_t *start = NULL;
    const wchar_t *end = NULL;
    if (!find_xml_section(xml, tag, &start, &end) || end <= start) {
        return false;
    }

    size_t value_len = (size_t)(end - start);
    if (value_len >= len) {
        value_len = len - 1;
    }
    wcsncpy(out, start, value_len);
    out[value_len] = L'\0';
    return true;
}

static void
trim_wstr_range(
    const wchar_t **start, const wchar_t **end)
{
    while (*start < *end && (**start == L' ' || **start == L'\t'
                              || **start == L'\r' || **start == L'\n')) {
        (*start)++;
    }
    while (*end > *start && ((*end)[-1] == L' ' || (*end)[-1] == L'\t'
                              || (*end)[-1] == L'\r' || (*end)[-1] == L'\n')) {
        (*end)--;
    }
}

static void
set_wstr_range(sentry_value_t object, const wchar_t *key_start, size_t key_len,
    const wchar_t *value_start, const wchar_t *value_end)
{
    if (key_len
            == (sizeof(SENTRY_WER_EVENT_ID_KEY_W) / sizeof(wchar_t)) - 1
        && wcsncmp(key_start, SENTRY_WER_EVENT_ID_KEY_W, key_len) == 0) {
        return;
    }

    trim_wstr_range(&value_start, &value_end);

    char *key = sentry__string_from_wstr_n(key_start, key_len);
    char *value = sentry__string_from_wstr_n(
        value_start, (size_t)(value_end - value_start));
    if (!sentry__string_empty(key) && value) {
        sentry_value_set_by_key(object, key, sentry_value_new_string(value));
    }
    sentry_free(key);
    sentry_free(value);
}

static void
set_wstring(sentry_value_t object, const char *key, const wchar_t *value)
{
    if (!value || !value[0]) {
        return;
    }

    sentry_value_t string = sentry__value_new_string_from_wstr(value);
    if (!sentry_value_is_null(string)) {
        sentry_value_set_by_key(object, key, string);
    }
}

static void
extract_xml_leaf_tags(
    sentry_value_t metadata, const wchar_t *start, const wchar_t *end)
{
    const wchar_t *cursor = start;
    while (cursor < end) {
        const wchar_t *open = find_wchar_before(cursor, end, L'<');
        if (!open || open + 1 >= end) {
            break;
        }

        if (open[1] == L'/' || open[1] == L'!' || open[1] == L'?') {
            cursor = open + 1;
            continue;
        }

        const wchar_t *tag_start = open + 1;
        const wchar_t *tag_end = tag_start;
        while (tag_end < end && *tag_end != L'>' && *tag_end != L' '
            && *tag_end != L'\t' && *tag_end != L'\r' && *tag_end != L'\n') {
            tag_end++;
        }
        if (tag_end == tag_start || tag_end >= end) {
            break;
        }

        const wchar_t *open_end = find_wchar_before(tag_end, end, L'>');
        if (!open_end) {
            break;
        }

        wchar_t close_tag[128];
        wchar_t tag[64];
        size_t tag_len = (size_t)(tag_end - tag_start);
        if (tag_len >= sizeof(tag) / sizeof(wchar_t)) {
            cursor = open_end + 1;
            continue;
        }
        wcsncpy(tag, tag_start, tag_len);
        tag[tag_len] = L'\0';
        if (!format_wstring(close_tag, sizeof(close_tag) / sizeof(wchar_t),
                L"</%s>", tag)) {
            cursor = open_end + 1;
            continue;
        }

        const wchar_t *value_start = open_end + 1;
        const wchar_t *close = wcsstr_before(value_start, close_tag, end);
        if (!close) {
            cursor = open_end + 1;
            continue;
        }

        if (!find_wchar_before(value_start, close, L'<')) {
            set_wstr_range(metadata, tag_start, tag_len, value_start, close);
        }

        cursor = close + wcslen(close_tag);
    }
}

static void
extract_xml_metadata_section(
    sentry_value_t metadata, const wchar_t *text, const wchar_t *tag)
{
    const wchar_t *start = NULL;
    const wchar_t *end = NULL;
    if (find_xml_section(text, tag, &start, &end)) {
        extract_xml_leaf_tags(metadata, start, end);
    }
}

static void
set_custom_metadata(sentry_value_t context, const wchar_t *text)
{
    sentry_value_t metadata = sentry_value_new_object();

    extract_xml_metadata_section(metadata, text, L"ProcessMetadata");

    if (sentry_value_get_length(metadata) > 0) {
        sentry_value_set_by_key(context, "metadata", metadata);
    } else {
        sentry_value_decref(metadata);
    }
}

static sentry_value_t
parse_internal_metadata(const wchar_t *text)
{
    if (!text || !wcsstr(text, L"<WERReportMetadata")) {
        return sentry_value_new_null();
    }

    sentry_value_t context = sentry_value_new_object();
    wchar_t report_id[64];
    if (copy_xml_tag(
            text, L"Guid", report_id, sizeof(report_id) / sizeof(wchar_t))) {
        set_wstring(context, "report_id", report_id);
    }
    set_custom_metadata(context, text);

    if (sentry_value_get_length(context) == 0) {
        sentry_value_decref(context);
        return sentry_value_new_null();
    }

    return context;
}

static sentry_value_t
read_temp_metadata(const wchar_t *path, const wchar_t *event_id_w)
{
    char *buffer = NULL;
    DWORD read = 0;
    if (!read_file(path, 256 * 1024, &buffer, &read)) {
        return sentry_value_new_null();
    }

    wchar_t *decoded = NULL;
    const wchar_t *text = NULL;
    bool decoded_buffer = decode_buffer(buffer, (size_t)read, &decoded, &text);
    sentry_free(buffer);
    if (!decoded_buffer) {
        return sentry_value_new_null();
    }

    if (!wcsstr(text, L"<WERReportMetadata")
        || !wcsstr(text, SENTRY_WER_EVENT_ID_KEY_W)
        || !wcsstr(text, event_id_w)) {
        sentry_free(decoded);
        return sentry_value_new_null();
    }

    sentry_value_t report = parse_internal_metadata(text);
    sentry_free(decoded);
    return report;
}

static sentry_value_t
read_temp_metadata_with_marker(const wchar_t *temp_dir, const char *event_id)
{
    if (sentry__string_empty(event_id)) {
        return sentry_value_new_null();
    }

    wchar_t *event_id_w = sentry__string_to_wstr(event_id);
    if (!event_id_w) {
        return sentry_value_new_null();
    }

    wchar_t pattern[MAX_PATH];
    if (!format_wstring(pattern, sizeof(pattern) / sizeof(wchar_t),
            L"%s\\*.WERInternalMetadata.xml", temp_dir)) {
        sentry_free(event_id_w);
        return sentry_value_new_null();
    }

    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) {
        sentry_free(event_id_w);
        return sentry_value_new_null();
    }

    sentry_value_t report = sentry_value_new_null();
    do {
        wchar_t path[MAX_PATH];
        if (!format_wstring(path, sizeof(path) / sizeof(wchar_t), L"%s\\%s",
                temp_dir, data.cFileName)) {
            continue;
        }

        report = read_temp_metadata(path, event_id_w);
        if (!sentry_value_is_null(report)) {
            break;
        }
    } while (FindNextFileW(find, &data));

    FindClose(find);
    sentry_free(event_id_w);

    return report;
}

sentry_value_t
sentry__wer_report_lookup(const char *event_id)
{
    if (sentry__string_empty(event_id)) {
        return sentry_value_new_null();
    }

    wchar_t program_data[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(
        L"ProgramData", program_data, sizeof(program_data) / sizeof(wchar_t));
    if (len == 0 || len >= sizeof(program_data) / sizeof(wchar_t)) {
        return sentry_value_new_null();
    }

    wchar_t temp_dir[MAX_PATH];
    if (!format_wstring(temp_dir, sizeof(temp_dir) / sizeof(wchar_t),
            L"%s\\Microsoft\\Windows\\WER\\Temp", program_data)) {
        return sentry_value_new_null();
    }

    sentry_value_t report = sentry_value_new_null();
    for (int i = 0; i < 20; i++) {
        if (i > 0) {
            Sleep(250);
        }

        report = read_temp_metadata_with_marker(temp_dir, event_id);
        if (!sentry_value_is_null(report)) {
            break;
        }
    }

    return report;
}

sentry_value_t
sentry__wer_report_from_buffer(const char *buffer, size_t size)
{
    wchar_t *decoded = NULL;
    const wchar_t *text = NULL;
    if (!decode_buffer(buffer, size, &decoded, &text)) {
        return sentry_value_new_null();
    }

    sentry_value_t report = parse_internal_metadata(text);
    sentry_free(decoded);
    return report;
}
