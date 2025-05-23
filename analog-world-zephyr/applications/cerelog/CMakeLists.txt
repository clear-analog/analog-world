cmake_minimum_required(VERSION 3.13.1)
set(BOARD esp32_devkitc/esp32/procpu)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(hello_world LANGUAGES C)

target_sources(app PRIVATE src/main.c)