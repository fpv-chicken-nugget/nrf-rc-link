cmake_minimum_required(VERSION 3.13)
project(nrf_rc_link C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(nrf_rc_link STATIC nrf_rc_link.c)

target_include_directories(nrf_rc_link PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
