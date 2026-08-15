set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(doom_firmware C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(doom_firmware)

target_sources(doom_firmware PRIVATE
    main.c
    lib/button/bootsel_button.c
)

add_subdirectory(lib/config)
add_subdirectory(lib/pwr_button)
add_subdirectory(lib/qspi_pio)
add_subdirectory(lib/amoled)
add_subdirectory(lib/gui)
add_subdirectory(lib/fonts)
add_subdirectory(lib/display)

include_directories(./lib/config)
include_directories(./lib/button)
include_directories(./lib/pwr_button)
include_directories(./lib/qspi_pio)
include_directories(./lib/amoled)
include_directories(./lib/gui)
include_directories(./lib/fonts)
include_directories(./lib/display)

target_link_libraries(doom_firmware PRIVATE
    config
    pwr_button
    display
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_dma
    m
)

pico_enable_stdio_usb(doom_firmware 1)
pico_enable_stdio_uart(doom_firmware 1)

pico_add_extra_outputs(doom_firmware)
