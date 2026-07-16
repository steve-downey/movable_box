# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(Catch2 3 REQUIRED)

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

include(Catch)
catch_discover_tests(beman.movable_box.tests.movable_box)

# Conformance cross-check: run the same suite against libstdc++'s
# exposition-only movable-box, std::ranges::__detail::__box. The paper claims
# the proposed surface is a strict subset of every shipping implementation;
# a semantic test failing here would be evidence against the wording. Needs
# libstdc++ (hence gated on GCC) and C++23, where P2494R2's movable-box
# semantics (move-only payloads) are available.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT BEMAN_MOVABLE_BOX_USE_MODULES)
    add_executable(beman.movable_box.tests.libstdcxx_box)
    target_sources(
        beman.movable_box.tests.libstdcxx_box
        PRIVATE movable_box.test.cpp
    )
    target_compile_definitions(
        beman.movable_box.tests.libstdcxx_box
        PRIVATE BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX=1
    )
    set_target_properties(
        beman.movable_box.tests.libstdcxx_box
        PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
    )
    target_link_libraries(
        beman.movable_box.tests.libstdcxx_box
        PRIVATE beman::movable_box Catch2::Catch2WithMain
    )
    catch_discover_tests(
        beman.movable_box.tests.libstdcxx_box
        TEST_SUFFIX " [libstdc++ __box]"
    )
endif()
