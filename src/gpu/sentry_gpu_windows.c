#include "sentry_gpu.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <dxgi.h>
#include <wbemidl.h>
#include <windows.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

static char *
wchar_to_utf8(const wchar_t *wstr)
{
    if (!wstr) {
        return NULL;
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return NULL;
    }

    char *str = sentry_malloc((size_t)len);
    if (!str) {
        return NULL;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL) <= 0) {
        sentry_free(str);
        return NULL;
    }

    return str;
}

static sentry_gpu_info_t *
get_gpu_info_dxgi(void)
{
    sentry_gpu_info_t *gpu_info = NULL;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;

    HRESULT hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    if (FAILED(hr)) {
        SENTRY_DEBUG("Failed to create DXGI factory");
        return NULL;
    }

    hr = factory->lpVtbl->EnumAdapters(factory, 0, &adapter);
    if (FAILED(hr)) {
        SENTRY_DEBUG("Failed to enumerate DXGI adapters");
        factory->lpVtbl->Release(factory);
        return NULL;
    }

    hr = adapter->lpVtbl->GetDesc(adapter, &desc);
    if (FAILED(hr)) {
        SENTRY_DEBUG("Failed to get DXGI adapter description");
        adapter->lpVtbl->Release(adapter);
        factory->lpVtbl->Release(factory);
        return NULL;
    }

    gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
    if (!gpu_info) {
        adapter->lpVtbl->Release(adapter);
        factory->lpVtbl->Release(factory);
        return NULL;
    }

    memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

    gpu_info->name = wchar_to_utf8(desc.Description);
    gpu_info->vendor_id = desc.VendorId;
    gpu_info->device_id = desc.DeviceId;
    gpu_info->memory_size = desc.DedicatedVideoMemory;
    gpu_info->vendor_name = sentry__gpu_vendor_id_to_name(desc.VendorId);

    adapter->lpVtbl->Release(adapter);
    factory->lpVtbl->Release(factory);

    return gpu_info;
}

sentry_gpu_info_t *
sentry__get_gpu_info(void)
{
    return get_gpu_info_dxgi();
}

void
sentry__free_gpu_info(sentry_gpu_info_t *gpu_info)
{
    if (!gpu_info) {
        return;
    }

    sentry_free(gpu_info->name);
    sentry_free(gpu_info->vendor_name);
    sentry_free(gpu_info->driver_version);
    sentry_free(gpu_info);
}
