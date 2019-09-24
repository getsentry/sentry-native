workspace "Sentry-Native"

SRC_ROOT = BREAKPAD_PKG

function breakpad_common()
  configurations { "Release" }
  language "C++"
  cppdialect "C++11"
  includedirs {
    SRC_ROOT.."/src",
  }

  flags {
  }

  filter "system:macosx or linux"
    toolset("clang")

  filter "system:macosx"
    defines {
      -- MacOS only?
      "HAVE_MACH_O_NLIST_H",
      "BPLOG_MINIMUM_SEVERITY=SEVERITY_ERROR",
    }
    links { "CoreFoundation.framework" }

  filter "system:linux"
    defines {
      "HAVE_A_OUT_H",
      "BPLOG_MINIMUM_SEVERITY=SEVERITY_ERROR",
    }

  filter "system:windows"
    includedirs {
      "$(VSInstallDir)/DIA SDK/include"
    }

  filter {}
end

-- project "breakpad_dump_syms"
--   kind "ConsoleApp"
--   breakpad_common()

--   filter "system:macosx"
--     files {
--       SRC_ROOT.."/src/common/*.c",
--       SRC_ROOT.."/src/common/*.cc",
--       SRC_ROOT.."/src/common/mac/*.cc",
--       SRC_ROOT.."/src/common/dwarf/*.cc",
--       SRC_ROOT.."/src/tools/mac/dump_syms/dump_syms_tool.cc",
--     }

--   filter "system:linux"
--     files {
--       SRC_ROOT.."/src/common/dwarf_cfi_to_module.cc",
--       SRC_ROOT.."/src/common/dwarf_cu_to_module.cc",
--       SRC_ROOT.."/src/common/dwarf_line_to_module.cc",
--       SRC_ROOT.."/src/common/dwarf_range_list_handler.cc",
--       SRC_ROOT.."/src/common/language.cc",
--       SRC_ROOT.."/src/common/module.cc",
--       SRC_ROOT.."/src/common/path_helper.cc",
--       SRC_ROOT.."/src/common/stabs_reader.cc",
--       SRC_ROOT.."/src/common/stabs_to_module.cc",
--       SRC_ROOT.."/src/common/dwarf/bytereader.cc",
--       SRC_ROOT.."/src/common/dwarf/dwarf2diehandler.cc",
--       SRC_ROOT.."/src/common/dwarf/dwarf2reader.cc",
--       SRC_ROOT.."/src/common/dwarf/elf_reader.cc",
--       SRC_ROOT.."/src/common/linux/crc32.cc",
--       SRC_ROOT.."/src/common/linux/dump_symbols.cc",
--       SRC_ROOT.."/src/common/linux/elf_symbols_to_module.cc",
--       SRC_ROOT.."/src/common/linux/elfutils.cc",
--       SRC_ROOT.."/src/common/linux/file_id.cc",
--       SRC_ROOT.."/src/common/linux/linux_libc_support.cc",
--       SRC_ROOT.."/src/common/linux/memory_mapped_file.cc",
--       SRC_ROOT.."/src/common/linux/safe_readlink.cc",
--       SRC_ROOT.."/src/tools/linux/dump_syms/dump_syms.cc",
--     }
--   filter "system:windows"
--     links {"imagehlp.lib", "diaguids.lib"}
--     files {
--       SRC_ROOT.."/src/common/windows/dia_util.cc",
--       SRC_ROOT.."/src/common/windows/guid_string.cc",
--       SRC_ROOT.."/src/common/windows/omap.cc",
--       SRC_ROOT.."/src/common/windows/pdb_source_line_writer.cc",
--       SRC_ROOT.."/src/common/windows/string_utils.cc",
--       SRC_ROOT.."/src/tools/windows/dump_syms/dump_syms.cc",
--     }
--   filter {"system:windows", "platforms:Win32"}
--     libdirs {"$(VSInstallDir)/DIA SDK/lib"}
--   filter {"system:windows", "platforms:Win64"}
--     libdirs {"$(VSInstallDir)/DIA SDK/lib/amd64"}

--   filter {}

--   removefiles {
--     SRC_ROOT.."/src/**/*_unittest.cc",
--     SRC_ROOT.."/src/**/*_test.cc",
--   }

-- project "breakpad_minidump_dump"
--   kind "ConsoleApp"
--   breakpad_common()

--   files {
--     SRC_ROOT.."/src/processor/minidump_dump.cc",
--     SRC_ROOT.."/src/processor/basic_code_modules.cc",
--     SRC_ROOT.."/src/processor/convert_old_arm64_context.cc",
--     SRC_ROOT.."/src/processor/dump_context.cc",
--     SRC_ROOT.."/src/processor/dump_object.cc",
--     SRC_ROOT.."/src/processor/logging.cc",
--     SRC_ROOT.."/src/processor/minidump.cc",
--     SRC_ROOT.."/src/processor/pathname_stripper.cc",
--     SRC_ROOT.."/src/processor/proc_maps_linux.cc",
--   }
--   removefiles {
--     SRC_ROOT.."/src/**/*_unittest.cc",
--     SRC_ROOT.."/src/**/*_test.cc",
--   }

--   filter "system:windows"
--     -- This project cannot be currently compiled on Windows
--     removefiles {SRC_ROOT.."/src/**"}

-- project "breakpad_minidump_stackwalk"
--   kind "ConsoleApp"
--   breakpad_common()

--   links {"breakpad_disasm"}

--   files {
--     SRC_ROOT.."/src/processor/minidump_stackwalk.cc",
--     SRC_ROOT.."/src/common/path_helper.cc",
--     SRC_ROOT.."/src/processor/basic_code_modules.cc",
--     SRC_ROOT.."/src/processor/basic_source_line_resolver.cc",
--     SRC_ROOT.."/src/processor/call_stack.cc",
--     SRC_ROOT.."/src/processor/cfi_frame_info.cc",
--     SRC_ROOT.."/src/processor/convert_old_arm64_context.cc",
--     SRC_ROOT.."/src/processor/disassembler_x86.cc",
--     SRC_ROOT.."/src/processor/dump_context.cc",
--     SRC_ROOT.."/src/processor/dump_object.cc",
--     SRC_ROOT.."/src/processor/exploitability.cc",
--     SRC_ROOT.."/src/processor/exploitability_linux.cc",
--     SRC_ROOT.."/src/processor/exploitability_win.cc",
--     SRC_ROOT.."/src/processor/logging.cc",
--     SRC_ROOT.."/src/processor/minidump.cc",
--     SRC_ROOT.."/src/processor/minidump_processor.cc",
--     SRC_ROOT.."/src/processor/pathname_stripper.cc",
--     SRC_ROOT.."/src/processor/process_state.cc",
--     SRC_ROOT.."/src/processor/proc_maps_linux.cc",
--     SRC_ROOT.."/src/processor/simple_symbol_supplier.cc",
--     SRC_ROOT.."/src/processor/source_line_resolver_base.cc",
--     SRC_ROOT.."/src/processor/stack_frame_cpu.cc",
--     SRC_ROOT.."/src/processor/stack_frame_symbolizer.cc",
--     SRC_ROOT.."/src/processor/stackwalk_common.cc",
--     SRC_ROOT.."/src/processor/stackwalker*.cc",
--     SRC_ROOT.."/src/processor/symbolic_constants_win.cc",
--     SRC_ROOT.."/src/processor/tokenize.cc",
--   }
--   removefiles {
--     SRC_ROOT.."/src/**/*_unittest.cc",
--     SRC_ROOT.."/src/**/*test.cc",
--   }

--   filter "system:windows"
--     -- This project cannot be currently compiled on Windows
--     removefiles {SRC_ROOT.."/src/**"}
--     removelinks {"*"}

-- project "breakpad_disasm"
--   kind "StaticLib"
--   breakpad_common()

--   pic "On"
--   files {
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_implicit.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_insn.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_invariant.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_modrm.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_opcode_tables.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_operand.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_reg.c",
--     SRC_ROOT.."/src/third_party/libdisasm/ia32_settings.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_disasm.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_format.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_imm.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_insn.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_misc.c",
--     SRC_ROOT.."/src/third_party/libdisasm/x86_operand_list.c",
--   }
--
--   filter "system:windows"
--   -- This project cannot be currently compiled on Windows
--     removefiles {SRC_ROOT.."/src/**"}

project "breakpad_client"
  kind "StaticLib"
  breakpad_common()

  pic "On"

  files {
    SRC_ROOT.."/src/common/string_conversion.cc",
    SRC_ROOT.."/src/common/md5.cc",
  }

  filter "system:macosx"
    files {
      SRC_ROOT.."/src/client/minidump_file_writer.cc",
      SRC_ROOT.."/src/client/mac/crash_generation/crash_generation_client.cc",
      SRC_ROOT.."/src/client/mac/handler/dynamic_images.cc",
      SRC_ROOT.."/src/client/mac/handler/exception_handler.cc",
      SRC_ROOT.."/src/client/mac/handler/minidump_generator.cc",
      SRC_ROOT.."/src/client/mac/handler/breakpad_nlist_64.cc",
      SRC_ROOT.."/src/common/convert_UTF.cc",
      SRC_ROOT.."/src/common/mac/bootstrap_compat.cc",
      SRC_ROOT.."/src/common/mac/file_id.cc",
      SRC_ROOT.."/src/common/mac/macho_id.cc",
      SRC_ROOT.."/src/common/mac/macho_utilities.cc",
      SRC_ROOT.."/src/common/mac/macho_walker.cc",
      SRC_ROOT.."/src/common/mac/string_utilities.cc",
      SRC_ROOT.."/src/common/mac/MachIPC.mm",
      SRC_ROOT.."/src/common/mac/HTTPMultipartUpload.m",
    }

  filter "system:linux"
    files {
      SRC_ROOT.."/src/client/linux/crash_generation/crash_generation_client.cc",
      SRC_ROOT.."/src/client/linux/crash_generation/crash_generation_server.cc",
      SRC_ROOT.."/src/client/linux/dump_writer_common/thread_info.cc",
      SRC_ROOT.."/src/client/linux/dump_writer_common/ucontext_reader.cc",
      SRC_ROOT.."/src/client/linux/handler/exception_handler.cc",
      SRC_ROOT.."/src/client/linux/handler/minidump_descriptor.cc",
      SRC_ROOT.."/src/client/linux/log/log.cc",
      SRC_ROOT.."/src/client/linux/microdump_writer/microdump_writer.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/linux_core_dumper.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/linux_dumper.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/linux_ptrace_dumper.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/minidump_writer.cc",
      SRC_ROOT.."/src/client/minidump_file_writer.cc",
      SRC_ROOT.."/src/common/convert_UTF.cc",
      SRC_ROOT.."/src/common/linux/elf_core_dump.cc",
      SRC_ROOT.."/src/common/linux/elfutils.cc",
      SRC_ROOT.."/src/common/linux/file_id.cc",
      SRC_ROOT.."/src/common/linux/guid_creator.cc",
      SRC_ROOT.."/src/common/linux/linux_libc_support.cc",
      SRC_ROOT.."/src/common/linux/memory_mapped_file.cc",
      SRC_ROOT.."/src/common/linux/safe_readlink.cc",
      SRC_ROOT.."/src/common/linux/http_upload.cc",
    }

  filter "system:windows"
    files {
      SRC_ROOT.."/src/client/windows/crash_generation/crash_generation_client.cc",
      SRC_ROOT.."/src/client/windows/handler/exception_handler.cc",
      SRC_ROOT.."/src/common/windows/dia_util.cc",
      SRC_ROOT.."/src/common/windows/guid_string.cc",
      SRC_ROOT.."/src/common/windows/http_upload.cc",
      SRC_ROOT.."/src/common/windows/pdb_source_line_writer.cc",
      SRC_ROOT.."/src/common/windows/string_utils.cc",
    }

  filter "system:android"
    includedirs {
      SRC_ROOT.."/src/common/android/include",
    }
    files {
      SRC_ROOT.."/src/client/linux/crash_generation/crash_generation_client.cc",
      SRC_ROOT.."/src/client/linux/dump_writer_common/thread_info.cc",
      SRC_ROOT.."/src/client/linux/dump_writer_common/ucontext_reader.cc",
      SRC_ROOT.."/src/client/linux/handler/exception_handler.cc",
      SRC_ROOT.."/src/client/linux/handler/minidump_descriptor.cc",
      SRC_ROOT.."/src/client/linux/log/log.cc",
      SRC_ROOT.."/src/client/linux/microdump_writer/microdump_writer.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/linux_dumper.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/linux_ptrace_dumper.cc",
      SRC_ROOT.."/src/client/linux/minidump_writer/minidump_writer.cc",
      SRC_ROOT.."/src/client/minidump_file_writer.cc",
      SRC_ROOT.."/src/common/android/breakpad_getcontext.S",
      SRC_ROOT.."/src/common/convert_UTF.cc",
      SRC_ROOT.."/src/common/linux/elfutils.cc",
      SRC_ROOT.."/src/common/linux/file_id.cc",
      SRC_ROOT.."/src/common/linux/guid_creator.cc",
      SRC_ROOT.."/src/common/linux/linux_libc_support.cc",
      SRC_ROOT.."/src/common/linux/memory_mapped_file.cc",
      SRC_ROOT.."/src/common/linux/safe_readlink.cc",
    }

EXAMPLES_DIR = "../breakpad/examples"

project "breakpad_crash"
  kind "ConsoleApp"
  breakpad_common()

  links {
    "breakpad_client",
  }

  filter "system:macosx"
    files {
      EXAMPLES_DIR.."/mac/crash.cc",
    }

  filter "system:linux"
    links {
      "pthread",
    }
    files {
      EXAMPLES_DIR.."/linux/crash.cc",
    }

  filter "system:windows"
    files {
      EXAMPLES_DIR.."/windows/crash.cc",
    }

  filter "system:android"
    links {
      "log",
    }
    files {
      EXAMPLES_DIR.."/linux/crash.cc",
    }

  filter {}
