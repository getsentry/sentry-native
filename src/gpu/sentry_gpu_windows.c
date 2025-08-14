#include "sentry_gpu.h"

#include "sentry_alloc.h"
#include "sentry_gpu_nvml.h"
#include "sentry_logger.h"
#include "sentry_string.h"

#include <dxgi.h>
#include <wbemidl.h>
#include <windows.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

sentry_gpu_list_t *
sentry__get_gpu_info(void)
{
    // First, try to get NVIDIA GPUs via NVML for enhanced info
    sentry_gpu_list_t *gpu_list = sentry__get_gpu_info_nvidia_nvml();
    if (!gpu_list) {
        // Didn't fidn any NVIDIA GPUs, let's use DXGI to check the rest
        gpu_list = sentry_malloc(sizeof(sentry_gpu_list_t));
        if (!gpu_list) {
            return NULL;
        }

        gpu_list->gpus = NULL;
        gpu_list->count = 0;
    }

    // Now use DXGI to find non-NVIDIA GPUs and add them to the list
    IDXGIFactory *factory = NULL;
    HRESULT hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    if (FAILED(hr)) {
        if (gpu_list->count == 0) {
            sentry_free(gpu_list);
            return NULL;
        }
        return gpu_list; // Return NVIDIA GPUs if we have them
    }

    // Count total adapters and non-NVIDIA adapters
    unsigned int adapter_count = 0;
    unsigned int non_nvidia_count = 0;
    IDXGIAdapter *temp_adapter = NULL;
    
    while (factory->lpVtbl->EnumAdapters(factory, adapter_count, &temp_adapter)
        != DXGI_ERROR_NOT_FOUND) {
        if (temp_adapter) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(temp_adapter->lpVtbl->GetDesc(temp_adapter, &desc))) {
                // Count non-NVIDIA GPUs, or all GPUs if no NVML GPUs were found
                if (desc.VendorId != 0x10de || gpu_list->count == 0) {
                    non_nvidia_count++;
                }
            }
            temp_adapter->lpVtbl->Release(temp_adapter);
            adapter_count++;
        }
    }

    if (non_nvidia_count > 0) {
        unsigned int nvidia_count = gpu_list->count;
        unsigned int total_count = nvidia_count + non_nvidia_count;
        
        // Expand or allocate the GPU array
        sentry_gpu_info_t **all_gpus = sentry_malloc(sizeof(sentry_gpu_info_t*) * total_count);
        if (!all_gpus) {
            factory->lpVtbl->Release(factory);
            return gpu_list; // Return what we have
        }
        
        // Copy existing NVIDIA GPUs if any
        for (unsigned int i = 0; i < nvidia_count; i++) {
            all_gpus[i] = gpu_list->gpus[i];
        }
        
        // Free old array (but keep the GPU info structs)
        sentry_free(gpu_list->gpus);
        gpu_list->gpus = all_gpus;
        
        // Enumerate adapters and add non-NVIDIA ones (or all if no NVIDIA found)
        for (unsigned int i = 0; i < adapter_count && gpu_list->count < total_count; i++) {
            IDXGIAdapter *adapter = NULL;
            DXGI_ADAPTER_DESC desc;

            hr = factory->lpVtbl->EnumAdapters(factory, i, &adapter);
            if (FAILED(hr)) {
                continue;
            }

            hr = adapter->lpVtbl->GetDesc(adapter, &desc);
            if (FAILED(hr)) {
                adapter->lpVtbl->Release(adapter);
                continue;
            }

            // Skip NVIDIA GPUs if we already have them via NVML
            if (desc.VendorId == 0x10de && nvidia_count > 0) {
                adapter->lpVtbl->Release(adapter);
                continue;
            }

            sentry_gpu_info_t *gpu_info = sentry_malloc(sizeof(sentry_gpu_info_t));
            if (!gpu_info) {
                adapter->lpVtbl->Release(adapter);
                continue;
            }

            memset(gpu_info, 0, sizeof(sentry_gpu_info_t));

            gpu_info->name = sentry__string_from_wstr(desc.Description);
            gpu_info->vendor_id = desc.VendorId;
            gpu_info->device_id = desc.DeviceId;
            gpu_info->memory_size = desc.DedicatedVideoMemory;
            gpu_info->vendor_name = sentry__gpu_vendor_id_to_name(desc.VendorId);

            gpu_list->gpus[gpu_list->count] = gpu_info;
            gpu_list->count++;

            adapter->lpVtbl->Release(adapter);
        }
    }

    factory->lpVtbl->Release(factory);

    if (gpu_list->count == 0) {
        sentry_free(gpu_list->gpus);
        sentry_free(gpu_list);
        return NULL;
    }

    return gpu_list;
}
