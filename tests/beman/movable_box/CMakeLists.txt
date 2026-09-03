# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(Catch2 3 REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)

add_executable(beman.movable_box.tests.movable_box)
target_sources(beman.movable_box.tests.movable_box PRIVATE movable_box.test.cpp)
target_link_libraries(
    beman.movable_box.tests.movable_box
    PRIVATE beman::movable_box Catch2::Catch2WithMain
)
if(BEMAN_MOVABLE_BOX_USE_MODULES)
    set_target_properties(
        beman.movable_box.tests.movable_box
        PROPERTIES CXX_MODULE_STD ON
    )
endif()

catch_discover_tests(beman.movable_box.tests.movable_box)

# Conformance cross-checks: run the same suite against each vendor's
# exposition-only movable-box (see box_under_test.hpp for the box selection
# and the per-divergence expected-failure catalog). The paper claims the
# proposed surface can be provided by every shipping implementation as its
# existing box; a semantic test failing here without an expected-failure
# tag would be evidence against the wording. Each target is gated on a
# probe for the standard library actually in use -- not on the compiler id,
# since e.g. clang builds against either libstdc++ or libc++ -- and needs
# C++23, where P2494R2's movable-box semantics (move-only payloads) are
# available.
if(NOT BEMAN_MOVABLE_BOX_USE_MODULES)
    include(CheckCXXSourceCompiles)

    block(SCOPE_FOR VARIABLES)
        # Probe with the standard the conformance targets compile under.
        # Every probe also requires __cpp_lib_ranges >= 202207L: that is
        # P2494R2's bump, so it is precisely "this library's box has
        # movable-box (move-only payload) semantics" -- e.g. libstdc++ 13's
        # __box is still constrained on copy_constructible and cannot host
        # the move-only parts of the suite.
        set(CMAKE_CXX_STANDARD 23)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)

        check_cxx_source_compiles(
            "#include <version>
             #if !defined(__GLIBCXX__) || __cpp_lib_ranges < 202207L
             #error not libstdc++ with movable-box semantics
             #endif
             int main() {}"
            BEMAN_MOVABLE_BOX_STDLIB_IS_LIBSTDCXX
        )
        # libc++ renamed __copyable_box to __movable_box (with P2494R2
        # semantics) in LLVM 17; the header probe keeps older libc++s out.
        check_cxx_source_compiles(
            "#include <version>
             #if !defined(_LIBCPP_VERSION) || __cpp_lib_ranges < 202207L \\
                 || !__has_include(<__ranges/movable_box.h>)
             #error not libc++ with movable-box semantics
             #endif
             int main() {}"
            BEMAN_MOVABLE_BOX_STDLIB_IS_LIBCXX
        )
        check_cxx_source_compiles(
            "#include <version>
             #if !defined(_MSVC_STL_VERSION) || __cpp_lib_ranges < 202207L
             #error not the MS STL with movable-box semantics
             #endif
             int main() {}"
            BEMAN_MOVABLE_BOX_STDLIB_IS_MSVC_STL
        )
    endblock()

    function(beman_movable_box_add_conformance_test name macro suffix)
        add_executable(${name})
        target_sources(${name} PRIVATE movable_box.test.cpp)
        target_compile_definitions(${name} PRIVATE ${macro}=1)
        set_target_properties(
            ${name}
            PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
        )
        target_link_libraries(
            ${name}
            PRIVATE beman::movable_box Catch2::Catch2WithMain
        )
        catch_discover_tests(${name} TEST_SUFFIX "${suffix}")
    endfunction()

    if(BEMAN_MOVABLE_BOX_STDLIB_IS_LIBSTDCXX)
        beman_movable_box_add_conformance_test(
            beman.movable_box.tests.libstdcxx_box
            BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX
            " [libstdc++ __box]"
        )
    endif()

    if(BEMAN_MOVABLE_BOX_STDLIB_IS_LIBCXX)
        beman_movable_box_add_conformance_test(
            beman.movable_box.tests.libcxx_box
            BEMAN_MOVABLE_BOX_TEST_LIBCXX_BOX
            " [libc++ __movable_box]"
        )
    endif()

    if(BEMAN_MOVABLE_BOX_STDLIB_IS_MSVC_STL)
        beman_movable_box_add_conformance_test(
            beman.movable_box.tests.msvc_stl_box
            BEMAN_MOVABLE_BOX_TEST_MSVC_BOX
            " [MS STL _Movable_box]"
        )
    endif()
endif()
