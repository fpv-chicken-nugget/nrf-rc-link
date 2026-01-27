cmake_minimum_required(VERSION 3.13)
project(nrf_rc_link C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(NRF_RC_LINK_SOURCES
        src/nrf_rc.c
        src/protocol.c
        src/link.c
        src/crc.c
        src/packet.c
)

set(NRF_RC_LINK_HEADERS
        include/nrf_rc.h
        include/protocol.h
        include/link.h
        include/crc.h
        include/packet.h
        include/config.h
)

add_library(nrf_rc_link STATIC
        ${NRF_RC_LINK_SOURCES}
        ${NRF_RC_LINK_HEADERS}
)

target_include_directories(nrf_rc_link PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
