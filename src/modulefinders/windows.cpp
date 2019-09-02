#include "base.hpp"
#ifdef SENTRY_WITH_WINDOWS_MODULEFINDER
#include <dbghelp.h>
#include <tlhelp32.h>
#include <windows.h>
#include <mutex>
#include <sstream>
#include "../uuid.hpp"

using namespace sentry;
using namespace modulefinders;

static std::mutex g_module_lock;
static Value g_modules;
static bool g_initialized;

#define CV_SIGNATURE 0x53445352

struct CodeViewRecord70 {
    uint32_t signature;
    GUID pdb_signature;
    uint32_t pdb_age;
    char pdb_filename[1];
};

static void extract_pdb_info(uintptr_t module_addr, Value &module) {
    if (!module_addr) {
        return;
    }

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)module_addr;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        return;
    }

    PIMAGE_NT_HEADERS nt_headers =
        (PIMAGE_NT_HEADERS)(module_addr + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        return;
    }

    uint32_t relative_addr =
        nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
            .VirtualAddress;
    if (!relative_addr) {
        return;
    }

    PIMAGE_DEBUG_DIRECTORY debug_dict =
        (PIMAGE_DEBUG_DIRECTORY)(module_addr + relative_addr);
    if (!debug_dict || debug_dict->Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
        return;
    }

    CodeViewRecord70 *debug_info = reinterpret_cast<CodeViewRecord70 *>(
        module_addr + debug_dict->AddressOfRawData);
    if (!debug_info || debug_info->signature != CV_SIGNATURE) {
        return;
    }

    module.set_by_key("debug_file", Value::new_string(debug_info->pdb_filename));

    sentry_uuid_t debug_id_base;
    debug_id_base.native_uuid = debug_info->pdb_signature;
    char id_buf[50];
    sentry_uuid_as_string(&debug_id_base, id_buf);
    id_buf[36] = '-';
    sprintf(id_buf + 37, "%x", debug_info->pdb_age);
    module.set_by_key("debug_id", Value::new_string(id_buf));

    sprintf(id_buf, "%08x%X", nt_headers->FileHeader.TimeDateStamp,
            nt_headers->OptionalHeader.SizeOfImage);
    module.set_by_key("code_id", Value::new_string(id_buf));
    module.set_by_key("type", Value::new_string("pe"));
}

static void load_modules() {
    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    MODULEENTRY32W module = {0};
    module.dwSize = sizeof(MODULEENTRY32W);
    g_modules = Value::new_list();

    if (Module32FirstW(snapshot, &module)) {
        do {
            HMODULE handle = LoadLibraryEx(module.szExePath, nullptr,
                                           LOAD_LIBRARY_AS_DATAFILE);
            MEMORY_BASIC_INFORMATION vmem_info = {0};
            if (handle &&
                sizeof(vmem_info) == VirtualQuery(module.modBaseAddr,
                                                  &vmem_info,
                                                  sizeof(vmem_info)) &&
                vmem_info.State == MEM_COMMIT) {
                Value rv = Value::new_object();
                rv.set_by_key(
                    "image_addr",
                    Value::new_addr((uint64_t)module.modBaseAddr));
                rv.set_by_key(
                    "image_size",
                    Value::new_int32((int32_t)module.modBaseSize));
                rv.set_by_key("code_file",
                                  Value::new_string(module.szExePath));
                extract_pdb_info((uintptr_t)module.modBaseAddr, rv);
                g_modules.append(rv);
            }
            FreeLibrary(handle);
        } while (Module32NextW(snapshot, &module));
    }

    CloseHandle(snapshot);
}

Value modulefinders::get_module_list() {
    std::lock_guard<std::mutex> _lock(g_module_lock);
    if (!g_initialized) {
        load_modules();
        g_initialized = true;
    }
    return g_modules;
}

#endif
