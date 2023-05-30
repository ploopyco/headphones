cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

include(pico_extras_import.cmake)

project(ploopy_headphones_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(ploopy_headphones
    run.c
    ringbuf.c
    i2s.c
    fix16.c
    bqf.c
    user.c
    configuration_manager.c
)

target_include_directories(ploopy_headphones PRIVATE ${CMAKE_SOURCE_DIR})

pico_generate_pio_header(ploopy_headphones ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

target_compile_definitions(ploopy_headphones PRIVATE
    # ours are zero based, so say so
    PICO_USBDEV_USE_ZERO_BASED_INTERFACES=1

    # need large descriptor
    PICO_USBDEV_MAX_DESCRIPTOR_SIZE=256

    PICO_USBDEV_ISOCHRONOUS_BUFFER_STRIDE_TYPE=1
)

pico_enable_stdio_usb(ploopy_headphones 0)
pico_enable_stdio_uart(ploopy_headphones 0)

pico_add_extra_outputs(ploopy_headphones)

target_link_libraries(ploopy_headphones
    hardware_pio
    hardware_clocks
    hardware_dma
    hardware_irq
    hardware_pwm
    hardware_i2c
    hardware_vreg
    hardware_sync
    pico_stdlib
    pico_multicore
    pico_unique_id
    usb_device
)
