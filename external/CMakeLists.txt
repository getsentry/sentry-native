if (LINUX OR ANDROID)
# The list is copied from: https://github.com/google/breakpad/blob/c7522272ffafa9b162f135aaee5d02a8895fcb0b/Makefile.am#L153-L188
    list(APPEND BREAKPAD_SOURCES
        src/client/linux/crash_generation/crash_generation_client.cc
        src/client/linux/crash_generation/crash_generation_server.cc
        src/client/linux/dump_writer_common/thread_info.cc
        src/client/linux/dump_writer_common/ucontext_reader.cc
        src/client/linux/handler/exception_handler.cc
        src/client/linux/handler/exception_handler.h
        src/client/linux/handler/minidump_descriptor.cc
        src/client/linux/handler/minidump_descriptor.h
        src/client/linux/log/log.cc
        src/client/linux/log/log.h
        src/client/linux/microdump_writer/microdump_writer.cc
        src/client/linux/microdump_writer/microdump_writer.h
        src/client/linux/minidump_writer/linux_core_dumper.cc
        src/client/linux/minidump_writer/linux_dumper.cc
        src/client/linux/minidump_writer/linux_ptrace_dumper.cc
        src/client/linux/minidump_writer/minidump_writer.cc
        src/client/minidump_file_writer-inl.h
        src/client/minidump_file_writer.cc
        src/client/minidump_file_writer.h
        src/common/convert_UTF.cc
        src/common/convert_UTF.h
        src/common/md5.cc
        src/common/md5.h
        src/common/string_conversion.cc
        src/common/string_conversion.h
        src/common/linux/elf_core_dump.cc
        src/common/linux/elfutils.cc
        src/common/linux/elfutils.h
        src/common/linux/file_id.cc
        src/common/linux/file_id.h
        src/common/linux/guid_creator.cc
        src/common/linux/guid_creator.h
        src/common/linux/linux_libc_support.cc
        src/common/linux/memory_mapped_file.cc
        src/common/linux/safe_readlink.cc
    )
endif()
if (ANDROID)
    list(APPEND BREAKPAD_SOURCES
        src/common/android/breakpad_getcontext.S
    )
endif()

PREPEND(BREAKPAD_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/breakpad/" ${BREAKPAD_SOURCES})

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
