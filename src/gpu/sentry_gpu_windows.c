#include "sentry_gpu.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <d3d9.h>
#include <dxgi.h>
#include <wbemidl.h>
#include <windows.h>

#pragma comment(lib, "d3d9.lib")
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

    char *str = sentry_malloc(len);
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

static sentry_gpu_info_t *
get_gpu_info_d3d9(void)
{
    sentry_gpu_info_t *gpu_info = NULL;
    LPDIRECT3D9 d3d = NULL;
    D3DADAPTER_IDENTIFIER9 adapter_id;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        SENTRY_DEBUG("Failed to create Direct3D9 object");
        return NULL;
    }

    HRESULT hr = d3d->lpVtbl->GetAdapterIdentifier(
        d3d, D3DADAPTER_DEFAULT, 0, &adapter_id);
    if (FAILED(hr)) {
        SENTRY_DEBUG("Failed to get D3D9 adapter identifier");
        d3d->lpVtbl->Release(d3d);
        return NULL;
    }

    gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
    if (!gpu_info) {
        d3d->lpVtbl->Release(d3d);
        return NULL;
    }

    memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

    gpu_info->name = sentry__string_clone(adapter_id.Description);
    gpu_info->vendor_id = adapter_id.VendorId;
    gpu_info->device_id = adapter_id.DeviceId;
    gpu_info->driver_version = sentry__string_clone(adapter_id.Driver);
    gpu_info->vendor_name = sentry__gpu_vendor_id_to_name(adapter_id.VendorId);

    d3d->lpVtbl->Release(d3d);

    return gpu_info;
}

sentry_gpu_info_t *
sentry__get_gpu_info(void)
{
    sentry_gpu_info_t *gpu_info = get_gpu_info_dxgi();
    if (!gpu_info) {
        gpu_info = get_gpu_info_d3d9();
    }
    return gpu_info;
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
