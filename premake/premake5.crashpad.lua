workspace "Sentry-Native"

SRC_ROOT = CRASHPAD_PKG
MIG_SCRIPT = SRC_ROOT..'/util/mach/mig.py'
-- FIXME: find dynamically?
MACOS_SYSROOT = '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk'
EXAMPLES_DIR = "../crashpad/examples"

function crashpad_common()
  language "C++"
  cppdialect "C++14"
  includedirs {
    SRC_ROOT,
    SRC_ROOT.."/third_party/mini_chromium/mini_chromium"
  }

  flags {
  }

  pic "on"

  filter "system:macosx"
    defines {
    }
    includedirs {
      SRC_ROOT.."/compat/mac",
      SRC_ROOT.."/compat/non_win",
      SRC_ROOT.."/compat/non_elf",
    }
    buildoptions {
      "-mmacosx-version-min=10.9"
    }

  filter "system:linux"
    toolset("clang")
    defines {
      "_FILE_OFFSET_BITS=64",
    }
    includedirs {
      SRC_ROOT.."/compat/linux",
      SRC_ROOT.."/compat/non_win",
      SRC_ROOT.."/compat/non_mac",
    }
    buildoptions {
      "-stdlib=libstdc++",
      -- for "clang":
      "-Wimplicit-fallthrough",
    }
    linkoptions {
      "-rtlib=libgcc",
      "-static-libstdc++",
      "-stdlib=libstdc++",
      "-Wl,--as-needed",
      "-Wl,-z,noexecstack",
    }
    linkgroups "on"
    filter {"system:linux", "kind:ConsoleApp"}
    linkoptions {
      "-pie",
    }

  filter "system:windows"
    includedirs {
      SRC_ROOT.."/compat/win",
      SRC_ROOT.."/compat/non_mac",
      SRC_ROOT.."/compat/non_elf",
    }

    disablewarnings {
      "4201",  -- nonstandard extension used : nameless struct/union
      "4996",  -- use of deprecated APIs: GetVersion, open
    }

    -- System stuff
    includedirs {
      "$(VSInstallDir)/DIA SDK/include"
    }
  filter {}
end

-- Temporary disable specific projects for Android by adding this function call
function disable_for_android()
  filter "system:android"
    kind "SharedLib"
    removefiles {
      SRC_ROOT.."/**",
      EXAMPLES_DIR.."/**",
    }
  filter {}
end

-- LSS is a header-only target in Crashpad that sets a define for dependants
function depend_lss()
  filter "system:linux"
    defines {
      -- required when including lss.h
      "CRASHPAD_LSS_SOURCE_EMBEDDED"
    }

  filter{}
end

-- aka "mini_chromium/base"
project "crashpad_minichromium_base"
  kind "StaticLib"
  crashpad_common()

  MINICHROMIUM_BASE_ROOT = SRC_ROOT.."/third_party/mini_chromium/mini_chromium/base"

  files {
    MINICHROMIUM_BASE_ROOT.."/debug/alias.cc",
    MINICHROMIUM_BASE_ROOT.."/files/file_path.cc",
    MINICHROMIUM_BASE_ROOT.."/files/scoped_file.cc",
    MINICHROMIUM_BASE_ROOT.."/logging.cc",
    MINICHROMIUM_BASE_ROOT.."/process/memory.cc",
    MINICHROMIUM_BASE_ROOT.."/rand_util.cc",
    MINICHROMIUM_BASE_ROOT.."/strings/string16.cc",
    MINICHROMIUM_BASE_ROOT.."/strings/string_number_conversions.cc",
    MINICHROMIUM_BASE_ROOT.."/strings/stringprintf.cc",
    MINICHROMIUM_BASE_ROOT.."/strings/utf_string_conversion_utils.cc",
    MINICHROMIUM_BASE_ROOT.."/strings/utf_string_conversions.cc",
    MINICHROMIUM_BASE_ROOT.."/synchronization/lock.cc",
    MINICHROMIUM_BASE_ROOT.."/third_party/icu/icu_utf.cc",
    MINICHROMIUM_BASE_ROOT.."/threading/thread_local_storage.cc",
  }

  filter "system:macosx"
    files {
      MINICHROMIUM_BASE_ROOT.."/mac/foundation_util.mm",
      MINICHROMIUM_BASE_ROOT.."/mac/close_nocancel.cc",
      MINICHROMIUM_BASE_ROOT.."/mac/mach_logging.cc",
      MINICHROMIUM_BASE_ROOT.."/mac/scoped_mach_port.cc",
      MINICHROMIUM_BASE_ROOT.."/mac/scoped_mach_vm.cc",
      MINICHROMIUM_BASE_ROOT.."/mac/scoped_nsautorelease_pool.mm",
      MINICHROMIUM_BASE_ROOT.."/strings/sys_string_conversions_mac.mm",
    }

  filter {"system:macosx or linux"}
    files {
      -- Posix
      MINICHROMIUM_BASE_ROOT.."/files/file_util_posix.cc",
      MINICHROMIUM_BASE_ROOT.."/posix/safe_strerror.cc",
      MINICHROMIUM_BASE_ROOT.."/process/process_metrics_posix.cc",
      MINICHROMIUM_BASE_ROOT.."/synchronization/lock_impl_posix.cc",
      MINICHROMIUM_BASE_ROOT.."/threading/thread_local_storage_posix.cc",
    }

  filter "system:windows"
    files {
      MINICHROMIUM_BASE_ROOT.."/process/process_metrics_win.cc",
      MINICHROMIUM_BASE_ROOT.."/strings/string_util_win.cc",
      MINICHROMIUM_BASE_ROOT.."/synchronization/lock_impl_win.cc",
      MINICHROMIUM_BASE_ROOT.."/threading/thread_local_storage_win.cc",
    }
    links {
      "advapi32.lib"
    }

  disable_for_android()

  filter {}

-- aka "client"
project "crashpad_client"
  kind "StaticLib"
  crashpad_common()
  depend_lss()

  files {
    SRC_ROOT.."/client/annotation.cc",
    SRC_ROOT.."/client/annotation_list.cc",
    SRC_ROOT.."/client/crash_report_database.cc",
    SRC_ROOT.."/client/crashpad_info.cc",
    SRC_ROOT.."/client/settings.cc",
    SRC_ROOT.."/client/prune_crash_reports.cc",
  }

  filter "system:macosx"
    files {
      SRC_ROOT.."/client/crash_report_database_mac.mm",
      SRC_ROOT.."/client/crashpad_client_mac.cc",
      SRC_ROOT.."/client/simulate_crash_mac.cc",
    }

  filter "system:linux"
    files {
      SRC_ROOT.."/client/crashpad_client_linux.cc",
      SRC_ROOT.."/client/client_argv_handling.cc",
      SRC_ROOT.."/client/crashpad_info_note.S",
      SRC_ROOT.."/client/crash_report_database_generic.cc",
    }

  filter "system:windows"
    files {
      SRC_ROOT.."/client/crash_report_database_win.cc",
      SRC_ROOT.."/client/crashpad_client_win.cc",
    }
    links {
      "rpcrt4.lib",
    }

  filter {}

  disable_for_android()

-- aka "util"
project "crashpad_util"
  kind "StaticLib"
  crashpad_common()
  depend_lss()

  files {
    SRC_ROOT.."/util/file/delimited_file_reader.cc",
    SRC_ROOT.."/util/file/file_io.cc",
    SRC_ROOT.."/util/file/file_reader.cc",
    SRC_ROOT.."/util/file/file_seeker.cc",
    SRC_ROOT.."/util/file/file_writer.cc",
    SRC_ROOT.."/util/file/scoped_remove_file.cc",
    SRC_ROOT.."/util/file/string_file.cc",
    SRC_ROOT.."/util/misc/initialization_state_dcheck.cc",
    SRC_ROOT.."/util/misc/lexing.cc",
    SRC_ROOT.."/util/misc/metrics.cc",
    SRC_ROOT.."/util/misc/pdb_structures.cc",
    SRC_ROOT.."/util/misc/random_string.cc",
    SRC_ROOT.."/util/misc/range_set.cc",
    SRC_ROOT.."/util/misc/reinterpret_bytes.cc",
    SRC_ROOT.."/util/misc/scoped_forbid_return.cc",
    SRC_ROOT.."/util/misc/time.cc",
    SRC_ROOT.."/util/misc/uuid.cc",
    SRC_ROOT.."/util/misc/zlib.cc",
    SRC_ROOT.."/util/net/http_body.cc",
    SRC_ROOT.."/util/net/http_body_gzip.cc",
    SRC_ROOT.."/util/net/http_multipart_builder.cc",
    SRC_ROOT.."/util/net/http_transport.cc",
    SRC_ROOT.."/util/net/url.cc",
    SRC_ROOT.."/util/numeric/checked_address_range.cc",
    SRC_ROOT.."/util/process/process_memory.cc",
    SRC_ROOT.."/util/process/process_memory_range.cc",
    SRC_ROOT.."/util/stdlib/aligned_allocator.cc",
    SRC_ROOT.."/util/stdlib/string_number_conversion.cc",
    SRC_ROOT.."/util/stdlib/strlcpy.cc",
    SRC_ROOT.."/util/stdlib/strnlen.cc",
    SRC_ROOT.."/util/string/split_string.cc",
    SRC_ROOT.."/util/thread/thread.cc",
    SRC_ROOT.."/util/thread/thread_log_messages.cc",
    SRC_ROOT.."/util/thread/worker_thread.cc",
  }

  filter {"system:macosx", "files:**/child_port.defs"}
    outputs = {
      'gen/util/mach/child_portUser.c',
      'gen/util/mach/child_portServer.c',
      'gen/util/mach/child_port.h',
      'gen/util/mach/child_portServer.h',
    }
    buildmessage 'Processing %{file.relpath}'
    buildcommands {
      '{MKDIR} gen/util/mach',
      string.format("python %s --include %s --sdk %s %s %s",
        MIG_SCRIPT,
        SRC_ROOT..'/compat/mac',
        MACOS_SYSROOT,
        '%{file.relpath}',
        table.concat(outputs, ' ')
      ),
    }
    buildoutputs { outputs }

  filter {"system:macosx", "files:**/notify.defs"}
    outputs = {
      'gen/util/mach/notifyUser.c',
      'gen/util/mach/notifyServer.c',
      'gen/util/mach/notify.h',
      'gen/util/mach/notifyServer.h',
    }
    buildmessage 'Processing %{file.relpath}'
    buildcommands {
      '{MKDIR} gen/util/mach',
      string.format("python %s --include %s --sdk %s %s %s",
        MIG_SCRIPT,
        SRC_ROOT..'/compat/mac',
        MACOS_SYSROOT,
        '%{file.relpath}',
        table.concat(outputs, ' ')
      ),
    }
    buildoutputs { outputs }

  filter {"system:macosx", "files:**/mach_exc.defs"}
    outputs = {
      'gen/util/mach/mach_excUser.c',
      'gen/util/mach/mach_excServer.c',
      'gen/util/mach/mach_exc.h',
      'gen/util/mach/mach_excServer.h',
    }
    buildmessage 'Processing %{file.relpath}'
    buildcommands {
      '{MKDIR} gen/util/mach',
      string.format("python %s --include %s --sdk %s %s %s",
        MIG_SCRIPT,
        SRC_ROOT..'/compat/mac',
        MACOS_SYSROOT,
        '%{file.relpath}',
        table.concat(outputs, ' ')
      ),
    }
    buildoutputs { outputs }

  filter {"system:macosx", "files:**/exc.defs"}
    outputs = {
      'gen/util/mach/excUser.c',
      'gen/util/mach/excServer.c',
      'gen/util/mach/exc.h',
      'gen/util/mach/excServer.h',
    }
    buildmessage 'Processing %{file.relpath}'
    buildcommands {
      '{MKDIR} gen/util/mach',
      string.format("python %s --include %s --sdk %s %s %s",
        MIG_SCRIPT,
        SRC_ROOT..'/compat/mac',
        MACOS_SYSROOT,
        '%{file.relpath}',
        table.concat(outputs, ' ')
      ),
    }
    buildoutputs { outputs }

  filter "system:macosx"
    defines {
      "CRASHPAD_ZLIB_SOURCE_SYSTEM"
    }

    gen_dir = "gen"

    includedirs {gen_dir}

    files {
      SRC_ROOT.."/util/mac/launchd.mm",
      SRC_ROOT.."/util/mac/mac_util.cc",
      SRC_ROOT.."/util/mac/service_management.cc",
      SRC_ROOT.."/util/mac/xattr.cc",
      SRC_ROOT.."/util/mach/child_port_handshake.cc",
      SRC_ROOT.."/util/mach/child_port_server.cc",
      SRC_ROOT.."/util/mach/composite_mach_message_server.cc",
      SRC_ROOT.."/util/mach/exc_client_variants.cc",
      SRC_ROOT.."/util/mach/exc_server_variants.cc",
      SRC_ROOT.."/util/mach/exception_behaviors.cc",
      SRC_ROOT.."/util/mach/exception_ports.cc",
      SRC_ROOT.."/util/mach/exception_types.cc",
      SRC_ROOT.."/util/mach/mach_extensions.cc",
      SRC_ROOT.."/util/mach/mach_message.cc",
      SRC_ROOT.."/util/mach/mach_message_server.cc",
      SRC_ROOT.."/util/mach/notify_server.cc",
      SRC_ROOT.."/util/mach/scoped_task_suspend.cc",
      SRC_ROOT.."/util/mach/symbolic_constants_mach.cc",
      SRC_ROOT.."/util/mach/task_for_pid.cc",
      SRC_ROOT.."/util/misc/capture_context_mac.S",
      SRC_ROOT.."/util/misc/clock_mac.cc",
      SRC_ROOT.."/util/misc/paths_mac.cc",
      SRC_ROOT.."/util/net/http_transport_mac.mm",
      SRC_ROOT.."/util/posix/process_info_mac.cc",
      SRC_ROOT.."/util/process/process_memory_mac.cc",
      SRC_ROOT.."/util/synchronization/semaphore_mac.cc",

      -- MIG inputs
      SRC_ROOT..'/util/mach/child_port.defs',
      MACOS_SYSROOT..'/usr/include/mach/notify.defs',
      MACOS_SYSROOT..'/usr/include/mach/mach_exc.defs',
      MACOS_SYSROOT..'/usr/include/mach/exc.defs',
      -- End MIG inputs
    }

  filter "system:linux"
    defines {
      "CRASHPAD_ZLIB_SOURCE_SYSTEM"
    }
    files {
      SRC_ROOT.."/util/net/http_transport_socket.cc",

      SRC_ROOT.."/util/linux/auxiliary_vector.cc",
      SRC_ROOT.."/util/linux/direct_ptrace_connection.cc",
      SRC_ROOT.."/util/linux/exception_handler_client.cc",
      SRC_ROOT.."/util/linux/exception_handler_protocol.cc",
      SRC_ROOT.."/util/linux/memory_map.cc",
      SRC_ROOT.."/util/linux/proc_stat_reader.cc",
      SRC_ROOT.."/util/linux/proc_task_reader.cc",
      SRC_ROOT.."/util/linux/ptrace_broker.cc",
      SRC_ROOT.."/util/linux/ptrace_client.cc",
      SRC_ROOT.."/util/linux/ptracer.cc",
      SRC_ROOT.."/util/linux/scoped_pr_set_dumpable.cc",
      SRC_ROOT.."/util/linux/scoped_pr_set_ptracer.cc",
      SRC_ROOT.."/util/linux/scoped_ptrace_attach.cc",
      SRC_ROOT.."/util/linux/socket.cc",
      SRC_ROOT.."/util/linux/thread_info.cc",
      SRC_ROOT.."/util/misc/capture_context_linux.S",
      SRC_ROOT.."/util/misc/paths_linux.cc",
      SRC_ROOT.."/util/posix/process_info_linux.cc",
      SRC_ROOT.."/util/process/process_memory_linux.cc",
      SRC_ROOT.."/util/process/process_memory_sanitized.cc",

      -- compat
      SRC_ROOT.."/compat/linux/sys/mman.cc"
    }

  filter {"system:macosx or linux"}
    files {
      -- Posix
      SRC_ROOT.."/util/file/directory_reader_posix.cc",
      SRC_ROOT.."/util/file/file_io_posix.cc",
      SRC_ROOT.."/util/file/filesystem_posix.cc",
      SRC_ROOT.."/util/misc/clock_posix.cc",
      SRC_ROOT.."/util/posix/close_stdio.cc",
      SRC_ROOT.."/util/posix/scoped_dir.cc",
      SRC_ROOT.."/util/posix/scoped_mmap.cc",
      SRC_ROOT.."/util/posix/signals.cc",
      SRC_ROOT.."/util/synchronization/semaphore_posix.cc",
      SRC_ROOT.."/util/thread/thread_posix.cc",
      SRC_ROOT.."/util/posix/close_multiple.cc",
      SRC_ROOT.."/util/posix/double_fork_and_exec.cc",
      SRC_ROOT.."/util/posix/drop_privileges.cc",
      SRC_ROOT.."/util/posix/symbolic_constants_posix.cc",
      -- End posix
    }

  filter "system:windows"
    defines {
      "CRASHPAD_ZLIB_SOURCE_EMBEDDED",
    }
    includedirs {
      SRC_ROOT.."/third_party/zlib",
    }

    files {
      SRC_ROOT.."/util/file/directory_reader_win.cc",
      SRC_ROOT.."/util/file/file_io_win.cc",
      SRC_ROOT.."/util/file/filesystem_win.cc",
      SRC_ROOT.."/util/misc/clock_win.cc",
      SRC_ROOT.."/util/misc/paths_win.cc",
      SRC_ROOT.."/util/misc/time_win.cc",
      SRC_ROOT.."/util/net/http_transport_win.cc",
      SRC_ROOT.."/util/process/process_memory_win.cc",
      SRC_ROOT.."/util/synchronization/semaphore_win.cc",
      SRC_ROOT.."/util/thread/thread_win.cc",
      SRC_ROOT.."/util/win/command_line.cc",
      SRC_ROOT.."/util/win/critical_section_with_debug_info.cc",
      SRC_ROOT.."/util/win/exception_handler_server.cc",
      SRC_ROOT.."/util/win/get_function.cc",
      SRC_ROOT.."/util/win/get_module_information.cc",
      SRC_ROOT.."/util/win/handle.cc",
      SRC_ROOT.."/util/win/initial_client_data.cc",
      SRC_ROOT.."/util/win/module_version.cc",
      SRC_ROOT.."/util/win/nt_internals.cc",
      SRC_ROOT.."/util/win/ntstatus_logging.cc",
      SRC_ROOT.."/util/win/process_info.cc",
      SRC_ROOT.."/util/win/registration_protocol_win.cc",
      SRC_ROOT.."/util/win/scoped_handle.cc",
      SRC_ROOT.."/util/win/scoped_local_alloc.cc",
      SRC_ROOT.."/util/win/scoped_process_suspend.cc",
      SRC_ROOT.."/util/win/scoped_set_event.cc",
      SRC_ROOT.."/util/win/session_end_watcher.cc",

      -- CaptureContext()
      SRC_ROOT.."/util/misc/capture_context_win.asm",
      SRC_ROOT.."/util/win/safe_terminate_process.asm",
    }
    links {
      "user32.lib",
      "version.lib",
      "winhttp.lib",
    }

  filter {"files:**.asm", "platforms:Win32"}
    exceptionhandling 'SEH'

  filter {}

  disable_for_android()

project "crashpad_snapshot"
  kind "StaticLib"
  crashpad_common()

  files {
    SRC_ROOT.."/snapshot/annotation_snapshot.cc",
    SRC_ROOT.."/snapshot/capture_memory.cc",
    SRC_ROOT.."/snapshot/cpu_context.cc",
    SRC_ROOT.."/snapshot/crashpad_info_client_options.cc",
    SRC_ROOT.."/snapshot/handle_snapshot.cc",
    SRC_ROOT.."/snapshot/memory_snapshot.cc",
    SRC_ROOT.."/snapshot/minidump/exception_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/minidump/memory_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/minidump/minidump_annotation_reader.cc",
    SRC_ROOT.."/snapshot/minidump/minidump_context_converter.cc",
    SRC_ROOT.."/snapshot/minidump/minidump_simple_string_dictionary_reader.cc",
    SRC_ROOT.."/snapshot/minidump/minidump_string_list_reader.cc",
    SRC_ROOT.."/snapshot/minidump/minidump_string_reader.cc",
    SRC_ROOT.."/snapshot/minidump/module_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/minidump/process_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/minidump/system_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/minidump/thread_snapshot_minidump.cc",
    SRC_ROOT.."/snapshot/unloaded_module_snapshot.cc",

    -- only for x86 and amd64
    SRC_ROOT.."/snapshot/x86/cpuid_reader.cc",
  }

  filter "system:macosx"
    files {
      SRC_ROOT.."/snapshot/mac/cpu_context_mac.cc",
      SRC_ROOT.."/snapshot/mac/exception_snapshot_mac.cc",
      SRC_ROOT.."/snapshot/mac/mach_o_image_annotations_reader.cc",
      SRC_ROOT.."/snapshot/mac/mach_o_image_reader.cc",
      SRC_ROOT.."/snapshot/mac/mach_o_image_segment_reader.cc",
      SRC_ROOT.."/snapshot/mac/mach_o_image_symbol_table_reader.cc",
      SRC_ROOT.."/snapshot/mac/module_snapshot_mac.cc",
      SRC_ROOT.."/snapshot/mac/process_reader_mac.cc",
      SRC_ROOT.."/snapshot/mac/process_snapshot_mac.cc",
      SRC_ROOT.."/snapshot/mac/process_types.cc",
      SRC_ROOT.."/snapshot/mac/process_types/all.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/annotation.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/crashpad_info.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/crashreporterclient.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/custom.cc",
      SRC_ROOT.."/snapshot/mac/process_types/dyld_images.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/loader.proctype",
      SRC_ROOT.."/snapshot/mac/process_types/nlist.proctype",
      SRC_ROOT.."/snapshot/mac/system_snapshot_mac.cc",
      SRC_ROOT.."/snapshot/mac/thread_snapshot_mac.cc",
    }

  filter "system:linux"
    files {
      SRC_ROOT.."/snapshot/linux/cpu_context_linux.cc",
      SRC_ROOT.."/snapshot/linux/debug_rendezvous.cc",
      SRC_ROOT.."/snapshot/linux/exception_snapshot_linux.cc",
      SRC_ROOT.."/snapshot/linux/process_reader_linux.cc",
      SRC_ROOT.."/snapshot/linux/process_snapshot_linux.cc",
      SRC_ROOT.."/snapshot/linux/system_snapshot_linux.cc",
      SRC_ROOT.."/snapshot/linux/thread_snapshot_linux.cc",
      SRC_ROOT.."/snapshot/sanitized/memory_snapshot_sanitized.cc",
      SRC_ROOT.."/snapshot/sanitized/module_snapshot_sanitized.cc",
      SRC_ROOT.."/snapshot/sanitized/process_snapshot_sanitized.cc",
      SRC_ROOT.."/snapshot/sanitized/sanitization_information.cc",
      SRC_ROOT.."/snapshot/sanitized/thread_snapshot_sanitized.cc",

      SRC_ROOT.."/snapshot/crashpad_types/crashpad_info_reader.cc",

      SRC_ROOT.."/snapshot/crashpad_types/image_annotation_reader.cc",
      SRC_ROOT.."/snapshot/elf/elf_dynamic_array_reader.cc",
      SRC_ROOT.."/snapshot/elf/elf_image_reader.cc",
      SRC_ROOT.."/snapshot/elf/elf_symbol_table_reader.cc",
      SRC_ROOT.."/snapshot/elf/module_snapshot_elf.cc",
    }

  filter {"system:macosx or linux"}
    files {
      -- Posix
      SRC_ROOT.."/snapshot/posix/timezone.cc",
      -- End posix
    }

  filter "system:windows"
    files {
      SRC_ROOT.."/snapshot/win/capture_memory_delegate_win.cc",
      SRC_ROOT.."/snapshot/win/cpu_context_win.cc",
      SRC_ROOT.."/snapshot/win/exception_snapshot_win.cc",
      SRC_ROOT.."/snapshot/win/memory_map_region_snapshot_win.cc",
      SRC_ROOT.."/snapshot/win/module_snapshot_win.cc",
      SRC_ROOT.."/snapshot/win/pe_image_annotations_reader.cc",
      SRC_ROOT.."/snapshot/win/pe_image_reader.cc",
      SRC_ROOT.."/snapshot/win/pe_image_resource_reader.cc",
      SRC_ROOT.."/snapshot/win/process_reader_win.cc",
      SRC_ROOT.."/snapshot/win/process_snapshot_win.cc",
      SRC_ROOT.."/snapshot/win/process_subrange_reader.cc",
      SRC_ROOT.."/snapshot/win/system_snapshot_win.cc",
      SRC_ROOT.."/snapshot/win/thread_snapshot_win.cc",

      SRC_ROOT.."/snapshot/crashpad_types/crashpad_info_reader.cc",
    }
    links {
      "powrprof.lib"
    }

  filter {}

  disable_for_android()

project "crashpad_minidump"
  kind "StaticLib"
  crashpad_common()

  files {
    SRC_ROOT.."/minidump/minidump_annotation_writer.cc",
    SRC_ROOT.."/minidump/minidump_byte_array_writer.cc",
    SRC_ROOT.."/minidump/minidump_context_writer.cc",
    SRC_ROOT.."/minidump/minidump_crashpad_info_writer.cc",
    SRC_ROOT.."/minidump/minidump_exception_writer.cc",
    SRC_ROOT.."/minidump/minidump_extensions.cc",
    SRC_ROOT.."/minidump/minidump_file_writer.cc",
    SRC_ROOT.."/minidump/minidump_handle_writer.cc",
    SRC_ROOT.."/minidump/minidump_memory_info_writer.cc",
    SRC_ROOT.."/minidump/minidump_memory_writer.cc",
    SRC_ROOT.."/minidump/minidump_misc_info_writer.cc",
    SRC_ROOT.."/minidump/minidump_module_crashpad_info_writer.cc",
    SRC_ROOT.."/minidump/minidump_module_writer.cc",
    SRC_ROOT.."/minidump/minidump_rva_list_writer.cc",
    SRC_ROOT.."/minidump/minidump_simple_string_dictionary_writer.cc",
    SRC_ROOT.."/minidump/minidump_stream_writer.cc",
    SRC_ROOT.."/minidump/minidump_string_writer.cc",
    SRC_ROOT.."/minidump/minidump_system_info_writer.cc",
    SRC_ROOT.."/minidump/minidump_thread_id_map.cc",
    SRC_ROOT.."/minidump/minidump_thread_writer.cc",
    SRC_ROOT.."/minidump/minidump_unloaded_module_writer.cc",
    SRC_ROOT.."/minidump/minidump_user_extension_stream_data_source.cc",
    SRC_ROOT.."/minidump/minidump_user_stream_writer.cc",
    SRC_ROOT.."/minidump/minidump_writable.cc",
    SRC_ROOT.."/minidump/minidump_writer_util.cc",
  }

  disable_for_android()

project "crashpad_handler"
  kind "ConsoleApp"
  crashpad_common()
  links {
    "crashpad_minichromium_base",
    "crashpad_client",
    "crashpad_util",
    "crashpad_snapshot",
    "crashpad_minidump",
  }

  files {
    SRC_ROOT.."/handler/main.cc",
    SRC_ROOT.."/tools/tool_support.cc",

    SRC_ROOT.."/handler/crash_report_upload_thread.cc",
    SRC_ROOT.."/handler/handler_main.cc",
    SRC_ROOT.."/handler/minidump_to_upload_parameters.cc",
    SRC_ROOT.."/handler/prune_crash_reports_thread.cc",
    SRC_ROOT.."/handler/user_stream_data_source.cc",
  }

  removefiles {
    SRC_ROOT.."/src/**/*_unittest.cc",
    SRC_ROOT.."/src/**/*_test.cc",
  }

  filter "system:macosx"
    files {
      SRC_ROOT.."/handler/mac/crash_report_exception_handler.cc",
      SRC_ROOT.."/handler/mac/exception_handler_server.cc",
      SRC_ROOT.."/handler/mac/file_limit_annotation.cc",
    }

    -- System stuff
    links {
      "ApplicationServices.framework",
      "CoreFoundation.framework",
      "Foundation.framework",
      "IOKit.framework",
      "Security.framework",
      "bsm",
      "z",
    }

  filter "system:linux"
    files {
      SRC_ROOT.."/handler/linux/capture_snapshot.cc",
      SRC_ROOT.."/handler/linux/crash_report_exception_handler.cc",
      SRC_ROOT.."/handler/linux/exception_handler_server.cc",
    }

    -- System stuff
    links {
      "pthread",
      "dl",
      "z",
    }

  filter "system:windows"
    -- Do not show console window when starting the handler
    kind "WindowedApp"
    files {
      SRC_ROOT.."/handler/win/crash_report_exception_handler.cc",

      -- compat
      SRC_ROOT.."/third_party/getopt/getopt.cc",
    }
    links {
      "crashpad_zlib",
    }

  filter {}

  disable_for_android()


project "crashpad_zlib"
  kind "StaticLib"
  crashpad_common()

  files {
    "./src/empty.c"
  }

  filter "system:windows"
    defines {
      "HAVE_STDARG_H"
    }

    disablewarnings {
      "4131",  -- uses old-style declarator
      "4244",  -- conversion from 't1' to 't2', possible loss of data
      "4245",  -- conversion from 't1' to 't2', signed/unsigned mismatch
      "4267",  -- conversion from 'size_t' to 't', possible loss of data
      "4324",  -- structure was padded due to alignment specifier
      "4702",  -- unreachable code
    }

    files {
      SRC_ROOT.."/third_party/zlib/zlib/adler32.c",
      SRC_ROOT.."/third_party/zlib/zlib/compress.c",
      SRC_ROOT.."/third_party/zlib/zlib/crc32.c",
      SRC_ROOT.."/third_party/zlib/zlib/deflate.c",
      SRC_ROOT.."/third_party/zlib/zlib/gzclose.c",
      SRC_ROOT.."/third_party/zlib/zlib/gzlib.c",
      SRC_ROOT.."/third_party/zlib/zlib/gzread.c",
      SRC_ROOT.."/third_party/zlib/zlib/gzwrite.c",
      SRC_ROOT.."/third_party/zlib/zlib/infback.c",
      SRC_ROOT.."/third_party/zlib/zlib/inffast.c",
      SRC_ROOT.."/third_party/zlib/zlib/inflate.c",
      SRC_ROOT.."/third_party/zlib/zlib/inftrees.c",
      SRC_ROOT.."/third_party/zlib/zlib/trees.c",
      SRC_ROOT.."/third_party/zlib/zlib/uncompr.c",
      SRC_ROOT.."/third_party/zlib/zlib/zutil.c",

      SRC_ROOT.."/third_party/zlib/zlib/crc_folding.c",
      SRC_ROOT.."/third_party/zlib/zlib/fill_window_sse.c",
      SRC_ROOT.."/third_party/zlib/zlib/x86.c",
    }


project "crashpad_crash"
  kind "ConsoleApp"
  crashpad_common()
  links {
    "crashpad_minichromium_base",
    "crashpad_util",
    "crashpad_client",
  }

  dependson {
    "crashpad_handler"
  }

  filter "system:macosx"
    files {
      EXAMPLES_DIR.."/mac/crash.cc",
    }

    -- System stuff
    links {
      "ApplicationServices.framework",
      "CoreFoundation.framework",
      "Foundation.framework",
      "IOKit.framework",
      "Security.framework",
      "bsm",
      "z",
    }

  filter "system:linux"
    files {
      EXAMPLES_DIR.."/linux/crash.cc",
    }
    links {
      "pthread",
      "dl",
    }

  filter "system:windows"
    files {
      EXAMPLES_DIR.."/windows/crash.cc",
    }

  filter {}

  disable_for_android()
