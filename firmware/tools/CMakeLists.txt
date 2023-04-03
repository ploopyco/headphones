cmake_minimum_required(VERSION 3.13)

project(ploopy_headphones_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(filter_test
    filter_test.c
    ../code/fix16.c
    ../code/bqf.c
    ../code/user.c
)

target_include_directories(filter_test PRIVATE ${CMAKE_SOURCE_DIR}/../code)

# TODO: user.c includes run.h to get the definition for SAMPLING_FREQ, but this
# pulls in the whole pico sdk as a dependency. A little refactoring would fix it all.
target_compile_definitions(filter_test PRIVATE
    SAMPLING_FREQ=48000
    RUN_H
)

target_link_libraries(filter_test
    m
)
