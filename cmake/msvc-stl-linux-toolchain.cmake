# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Cross-compile from Linux for x86_64 Windows with clang-cl and the
# Microsoft STL, so the MS STL movable-box conformance target
# (beman.movable_box.tests.msvc_stl_box) can be developed without a
# Windows machine. The MS STL has no native Linux support; this is
# ordinary cross-compilation against the MSVC CRT and Windows SDK, which
# xwin (https://github.com/Jake-Shadle/xwin) can download on Linux:
#
#   xwin --accept-license --arch x86_64 splat --output ~/xwin
#
# Then configure with:
#
#   cmake -B build/msvc-stl -S . -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-stl-linux-toolchain.cmake \
#     -DXWIN_DIR=$HOME/xwin \
#     -DCMAKE_CXX_STANDARD=23 \
#     -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=infra/cmake/use-fetch-content.cmake
#
# Building the test targets already runs every STATIC_REQUIRE, constexpr
# static_assert, and shim-parity check at compile time, which is most of
# the conformance value. To also run the test binaries under ctest,
# install Wine and add:
#
#   -DCMAKE_CROSSCOMPILING_EMULATOR=wine64
#
# This toolchain is a development convenience and is not exercised in CI;
# the Windows CI jobs remain the authority for the MS STL target.

include_guard(GLOBAL)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT DEFINED XWIN_DIR)
    if(DEFINED ENV{XWIN_DIR})
        set(XWIN_DIR "$ENV{XWIN_DIR}")
    else()
        message(
            FATAL_ERROR
            "XWIN_DIR is not set. Point it at an 'xwin splat' output "
            "directory (containing crt/ and sdk/); see the comment at the "
            "top of this toolchain file."
        )
    endif()
endif()

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_LINKER lld-link)
set(CMAKE_MT llvm-mt)

# clang-cl and lld-link both understand xwin's directory layout directly
# through /vctoolsdir and /winsdkdir, which set up the MSVC CRT and
# Windows SDK include and library paths.
set(_msvc_stl_sysroot_flags
    "/vctoolsdir \"${XWIN_DIR}/crt\" /winsdkdir \"${XWIN_DIR}/sdk\""
)

set(CMAKE_C_FLAGS_INIT "${_msvc_stl_sysroot_flags} /EHsc /permissive-")
set(CMAKE_CXX_FLAGS_INIT "${_msvc_stl_sysroot_flags} /EHsc /permissive-")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_msvc_stl_sysroot_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_msvc_stl_sysroot_flags}")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "")

# Everything Windows-shaped comes from XWIN_DIR; keep host paths out of
# find_* results.
set(CMAKE_FIND_ROOT_PATH "${XWIN_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
