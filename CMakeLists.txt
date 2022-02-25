cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_pdp8 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico_pdp8
    main.cpp
    nano8.cpp
    tti_iot.cpp
    tto_iot.cpp
    pti_iot.cpp
    pto_iot.cpp
    dsk_iot.cpp
    rk8e_iot.cpp
    osint.cpp
)

target_link_libraries(pico_pdp8 pico_stdlib)

pico_enable_stdio_usb(pico_pdp8 1)
pico_enable_stdio_uart(pico_pdp8 0)
pico_add_extra_outputs(pico_pdp8)
