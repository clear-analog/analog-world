# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(BOARD esp32_devkitc/esp32/procpu)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ads1299_acquisition LANGUAGES C)

# Add source files
target_sources(app PRIVATE
    src/main.c
    src/ads1299.c
    src/data_handler.c
)

# Add include directories
target_include_directories(app PRIVATE
    src/
)