# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(BEMAN_MOVABLE_BOX_USE_MODULES)
    target_sources(
        beman.movable_box
        PUBLIC
            FILE_SET CXX_MODULES FILES movable_box.cppm
            FILE_SET HEADERS
                FILES
                    config.hpp
                    detail/storage.hpp
                    movable_box.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/movable_box/config_generated.hpp"
    )
else()
    target_sources(
        beman.movable_box
        PUBLIC
            FILE_SET HEADERS
                FILES
                    config.hpp
                    detail/storage.hpp
                    movable_box.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/movable_box/config_generated.hpp"
    )
endif()
