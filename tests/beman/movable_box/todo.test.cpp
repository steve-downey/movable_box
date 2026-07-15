// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/movable_box/config.hpp>
#include <catch2/catch_all.hpp>
#include <beman/movable_box/todo.hpp>

TEST_CASE("todo", "[movable_box::todo]") {
    const bool todo = true;
    CHECK(todo);
}
