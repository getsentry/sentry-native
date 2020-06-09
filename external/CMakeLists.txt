if (LINUX OR ANDROID)
# The list is copied from: https://github.com/google/breakpad/blob/c7522272ffafa9b162f135aaee5d02a8895fcb0b/Makefile.am#L153-L188
    list(APPEND BREAKPAD_SOURCES
        breakpad/src/client/linux/crash_generation/crash_generation_client.cc
        breakpad/src/client/linux/crash_generation/crash_generation_server.cc
        breakpad/src/client/linux/dump_writer_common/thread_info.cc
        breakpad/src/client/linux/dump_writer_common/ucontext_reader.cc
        breakpad/src/client/linux/handler/exception_handler.cc
        breakpad/src/client/linux/handler/exception_handler.h
        breakpad/src/client/linux/handler/minidump_descriptor.cc
        breakpad/src/client/linux/handler/minidump_descriptor.h
        breakpad/src/client/linux/log/log.cc
        breakpad/src/client/linux/log/log.h
        breakpad/src/client/linux/microdump_writer/microdump_writer.cc
        breakpad/src/client/linux/microdump_writer/microdump_writer.h
        breakpad/src/client/linux/minidump_writer/linux_core_dumper.cc
        breakpad/src/client/linux/minidump_writer/linux_dumper.cc
        breakpad/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
        breakpad/src/client/linux/minidump_writer/minidump_writer.cc
        breakpad/src/client/minidump_file_writer-inl.h
        breakpad/src/client/minidump_file_writer.cc
        breakpad/src/client/minidump_file_writer.h
        breakpad/src/common/convert_UTF.cc
        breakpad/src/common/convert_UTF.h
        breakpad/src/common/md5.cc
        breakpad/src/common/md5.h
        breakpad/src/common/string_conversion.cc
        breakpad/src/common/string_conversion.h
        breakpad/src/common/linux/elf_core_dump.cc
        breakpad/src/common/linux/elfutils.cc
        breakpad/src/common/linux/elfutils.h
        breakpad/src/common/linux/file_id.cc
        breakpad/src/common/linux/file_id.h
        breakpad/src/common/linux/guid_creator.cc
        breakpad/src/common/linux/guid_creator.h
        breakpad/src/common/linux/linux_libc_support.cc
        breakpad/src/common/linux/memory_mapped_file.cc
        breakpad/src/common/linux/safe_readlink.cc
    )
endif()
if (ANDROID)
    list(APPEND BREAKPAD_SOURCES
        breakpad/src/common/android/breakpad_getcontext.S
    )
endif()
if( WIN32 )
    list(APPEND BREAKPAD_SOURCES
        breakpad/src/common/windows/guid_string.cc
        breakpad/src/common/windows/http_upload.cc
        breakpad/src/common/windows/string_utils.cc
        breakpad/src/client/windows/crash_generation/client_info.cc
        breakpad/src/client/windows/crash_generation/crash_generation_server.cc
        breakpad/src/client/windows/crash_generation/crash_generation_client.cc
        breakpad/src/client/windows/handler/exception_handler.cc
        breakpad/src/client/windows/sender/crash_report_sender.cc
        breakpad/src/processor/basic_code_modules.cc
        breakpad/src/processor/convert_old_arm64_context.cc
        breakpad/src/processor/dump_context.cc
        breakpad/src/processor/dump_object.cc
        breakpad/src/processor/logging.cc
        breakpad/src/processor/minidump.cc
        breakpad/src/processor/pathname_stripper.cc
        breakpad/src/processor/proc_maps_linux.cc
    )
endif()

add_library(breakpad_client STATIC ${BREAKPAD_SOURCES})
set_property(TARGET breakpad_client PROPERTY POSITION_INDEPENDENT_CODE ON)
# breakpad has includes directly to `third_party/lss/...`,
# which are being resolved correctly when we add the current directory to
# the include directories. A giant hack, yes, but it works
target_include_directories(breakpad_client
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/breakpad/src/>"
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
if(WIN32)
    target_compile_definitions(breakpad_client PRIVATE -DUNICODE)
    target_link_libraries(breakpad_client psapi)
endif()