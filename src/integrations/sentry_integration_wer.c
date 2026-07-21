#include "sentry_integration_wer.h"

#include "sentry_alloc.h"
#include "sentry_attachment.h"
#include "sentry_core.h"
#include "sentry_logger.h"
#include "sentry_scope.h"
#include "sentry_string.h"

#include <string.h>
#include <werapi.h>

// Windows 8+ SDK
#ifndef WER_FILE_ANONYMOUS_DATA
#    define WER_FILE_ANONYMOUS_DATA 0x2
#endif

typedef struct sentry_integration_wer_data_s {
    HRESULT(WINAPI *WerRegisterCustomMetadata)(PCWSTR, PCWSTR);
    HRESULT(WINAPI *WerUnregisterCustomMetadata)(PCWSTR);
    sentry_scope_observer_t *observer;
} sentry_integration_wer_data_t;

static void
wer_init(sentry_integration_wer_data_t *wer_data)
{
    if (wer_data->WerRegisterCustomMetadata) {
        return;
    }
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (kernel32) {
        wer_data->WerRegisterCustomMetadata = (HRESULT(WINAPI *)(PCWSTR,
            PCWSTR))GetProcAddress(kernel32, "WerRegisterCustomMetadata");
        wer_data->WerUnregisterCustomMetadata = (HRESULT(WINAPI *)(
            PCWSTR))GetProcAddress(kernel32, "WerUnregisterCustomMetadata");
    }
    if (!wer_data->WerRegisterCustomMetadata) {
        SENTRY_DEBUG("WerRegisterCustomMetadata not available; "
                     "tag sync to WER will be skipped");
    }
}

static void
sentry_integration_wer_free(void *data)
{
    sentry_free(data);
}

static void
wer_set_tag(void *data, const char *key, const char *value)
{
    sentry_integration_wer_data_t *wer_data
        = (sentry_integration_wer_data_t *)data;
    if (!wer_data->WerRegisterCustomMetadata) {
        return;
    }
    if (!key || !value) {
        return;
    }

    wchar_t *key_w = sentry__string_to_wstr(key);
    wchar_t *value_w = sentry__string_to_wstr(value);
    if (!key_w || !value_w) {
        sentry_free(key_w);
        sentry_free(value_w);
        return;
    }

    HRESULT hr = wer_data->WerRegisterCustomMetadata(key_w, value_w);
    if (FAILED(hr)) {
        SENTRY_WARNF(
            "WerRegisterCustomMetadata failed: hr=0x%08lx", (unsigned long)hr);
    }

    sentry_free(value_w);
    sentry_free(key_w);
}

static void
wer_remove_tag(void *data, const char *key)
{
    sentry_integration_wer_data_t *wer_data
        = (sentry_integration_wer_data_t *)data;
    if (!wer_data->WerUnregisterCustomMetadata) {
        return;
    }
    if (!key) {
        return;
    }

    wchar_t *key_w = sentry__string_to_wstr(key);
    if (!key_w) {
        return;
    }

    HRESULT hr = wer_data->WerUnregisterCustomMetadata(key_w);
    if (FAILED(hr)) {
        SENTRY_WARNF("WerUnregisterCustomMetadata failed: hr=0x%08lx",
            (unsigned long)hr);
    }

    sentry_free(key_w);
}

static void
wer_add_attachment(void *UNUSED(data), sentry_attachment_t *attachment)
{
    if (!attachment) {
        return;
    }

    if (attachment->path) {
        const wchar_t *path_w = attachment->path->path_w;
        if (!path_w) {
            return;
        }
        HRESULT hr = WerRegisterFile(
            path_w, WerRegFileTypeOther, WER_FILE_ANONYMOUS_DATA);
        if (FAILED(hr)) {
            SENTRY_WARNF(
                "WerRegisterFile failed: hr=0x%08lx", (unsigned long)hr);
        }
        return;
    }

    if (attachment->buf && attachment->buf_len > 0) {
        if (attachment->buf_len > MAXDWORD) {
            SENTRY_WARNF("WerRegisterMemoryBlock: buffer too large (%zu bytes)",
                attachment->buf_len);
            return;
        }
        HRESULT hr = WerRegisterMemoryBlock(
            (PVOID)attachment->buf, (DWORD)attachment->buf_len);
        if (FAILED(hr)) {
            SENTRY_WARNF(
                "WerRegisterMemoryBlock failed: hr=0x%08lx", (unsigned long)hr);
        }
        return;
    }
}

static void
wer_remove_attachment(void *UNUSED(data), sentry_attachment_t *attachment)
{
    if (!attachment) {
        return;
    }

    if (attachment->path) {
        const wchar_t *path_w = attachment->path->path_w;
        if (!path_w) {
            return;
        }
        HRESULT hr = WerUnregisterFile(path_w);
        if (FAILED(hr)) {
            SENTRY_WARNF(
                "WerUnregisterFile failed: hr=0x%08lx", (unsigned long)hr);
        }
        return;
    }

    if (attachment->buf) {
        HRESULT hr = WerUnregisterMemoryBlock((PVOID)attachment->buf);
        if (FAILED(hr)) {
            SENTRY_WARNF("WerUnregisterMemoryBlock failed: hr=0x%08lx",
                (unsigned long)hr);
        }
    }
}

static void
wer_cleanup_tag(const char *key, sentry_value_t UNUSED(value), void *data)
{
    wer_remove_tag(data, key);
}

static void
register_wer(
    void *data, sentry_scope_t *scope, const sentry_options_t *UNUSED(options))
{
    sentry_integration_wer_data_t *wer_data
        = (sentry_integration_wer_data_t *)data;
    wer_init(wer_data);

    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    if (!observer) {
        return;
    }
    observer->set_tag = wer_set_tag;
    observer->remove_tag = wer_remove_tag;
    observer->add_attachment = wer_add_attachment;
    observer->remove_attachment = wer_remove_attachment;
    observer->data = wer_data;

    if (sentry__scope_add_observer(scope, observer)) {
        wer_data->observer = observer;
        for (sentry_attachment_t *attachment = scope->attachments; attachment;
            attachment = attachment->next) {
            wer_add_attachment(wer_data, attachment);
        }
    }
}

static void
unregister_wer(
    void *data, sentry_scope_t *scope, const sentry_options_t *UNUSED(options))
{
    sentry_integration_wer_data_t *wer_data
        = (sentry_integration_wer_data_t *)data;
    if (!wer_data->observer) {
        return;
    }

    sentry__value_foreach_key_value(scope->tags, wer_cleanup_tag, wer_data);

    for (sentry_attachment_t *attachment = scope->attachments; attachment;
        attachment = attachment->next) {
        wer_remove_attachment(wer_data, attachment);
    }

    sentry__scope_remove_observer(scope, wer_data->observer);
    wer_data->observer = NULL;
}

sentry_integration_t *
sentry_integration_wer_new(void)
{
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    if (!integration) {
        return NULL;
    }

    integration->data = SENTRY_MAKE(sentry_integration_wer_data_t);
    if (!integration->data) {
        sentry_free(integration);
        return NULL;
    }

    integration->register_func = register_wer;
    integration->unregister_func = unregister_wer;
    integration->free_func = sentry_integration_wer_free;

    return integration;
}
