# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES movable_box)
message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.movable_box.examples.${example})
    target_sources(beman.movable_box.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(
        beman.movable_box.examples.${example}
        PRIVATE beman::movable_box
    )
    if(BEMAN_MOVABLE_BOX_USE_MODULES)
        set_target_properties(
            beman.movable_box.examples.${example}
            PROPERTIES CXX_MODULE_STD ON
        )
    endif()
    if(BEMAN_MOVABLE_BOX_BUILD_TESTS)
        add_test(
            NAME beman.movable_box.examples.${example}
            COMMAND beman.movable_box.examples.${example}
        )
    endif()
endforeach()
