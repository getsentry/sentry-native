if(NOT DEFINED ENV{MINGW64_ROOT})
    message(FATAL_ERROR "Required ENV variable MINGW64_ROOT does not exists. Please check README.md for more details !")
else()
    SET(MINGW64_ROOT $ENV{MINGW64_ROOT})
endif()

# search for programs in the build host directories
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)

# for libraries and headers in the target directories
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

list(APPEND CMAKE_PREFIX_PATH
    ${MINGW64_ROOT}/x86_64-w64-mingw32
    ${MINGW64_ROOT}
)

SET (CMAKE_ASM_MASM_COMPILER    "uasm")
SET (CMAKE_C_COMPILER           "clang")
SET (CMAKE_CXX_COMPILER         "clang++")

SET (CMAKE_C_FLAGS              "-fuse-ld=lld")
SET (CMAKE_CXX_FLAGS            ${CMAKE_C_FLAGS})

SET (CMAKE_C_FLAGS_DEBUG        "-O0 -g")
SET (CMAKE_CXX_FLAGS_DEBUG      ${CMAKE_C_FLAGS_DEBUG})
