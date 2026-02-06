cmake_minimum_required(VERSION 3.13)
project(nrf_rc_link C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(nrf_rc_link STATIC
        src/crc.c
        src/nrf_rc_driver.c
        drivers/nrf24.c
        include/config.h
        include/crc.h
        include/nrf24_config.h
        include/nrf_rc_driver.h
        drivers/include/nrf24.h
        drivers/include/nrf24_registers.h
)

target_include_directories(nrf_rc_link PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/include
)
