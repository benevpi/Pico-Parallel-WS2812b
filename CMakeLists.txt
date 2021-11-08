cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(christmas_tree)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(pico_parallel_ws2812b)

pico_generate_pio_header(pico_parallel_ws2812b ${CMAKE_CURRENT_LIST_DIR}/pico_parallel_ws2812b.pio)

target_sources(pico_parallel_ws2812b PRIVATE pico_parallel_ws2812b.c)

target_link_libraries(pico_parallel_ws2812b PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)

pico_add_extra_outputs(pico_parallel_ws2812b)
