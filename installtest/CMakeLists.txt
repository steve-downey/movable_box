# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30)

project(beman.movable_box.installtest LANGUAGES CXX)

find_package(beman.movable_box REQUIRED)

enable_testing()

add_executable(beman.movable_box.installtest ../examples/movable_box.cpp)
target_compile_features(beman.movable_box.installtest PRIVATE cxx_std_23)
target_link_libraries(beman.movable_box.installtest PRIVATE beman::movable_box)

add_test(
    NAME beman.movable_box.installtest
    COMMAND beman.movable_box.installtest
)
