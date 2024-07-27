# ==================================================================================================
# OPEN STORY TELLER MAIN CMAKE BUILD FILE
# (see README.md for invokable options)
# Default target: TBD ?
# ==================================================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)
set(PROJECT_NAME picoclock)

# ==================================================================================================
# DEFAULT VARIABLES OPTIONS
# ==================================================================================================
if(NOT OST_BUNDLE)
    set(OST_BUNDLE "RASPI_PICO")
endif()

if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "../pico-sdk")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ==================================================================================================
# OST COMMON FILES
# ==================================================================================================
set(OST_SRCS
    system/main.c
    system/sdcard.c
    system/ff_diskio_sdcard.c
    system/debug.c
    system/filesystem.c
    system/sdcard.c
    system/qor.c
    system/qor_armv6m.s
    system/audio_player.c
    system/ui_task.c
    #system/fs_task.c
    system/ff/ff.c
    system/ff/ffsystem.c
    system/ff/ff_stubs.c
    chip32/chip32_vm.c
    library/mini_qoi.c
)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library
    ${CMAKE_CURRENT_SOURCE_DIR}/system/ff
    ${CMAKE_CURRENT_SOURCE_DIR}/chip32
    ${CMAKE_CURRENT_SOURCE_DIR}/system/
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/e-Paper
)

message("=============  TARGET: RASPBERRY PICO  =============")

add_definitions(-DOST_USE_FF_LIBRARY)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(${CMAKE_CURRENT_SOURCE_DIR}/platform/raspberry-pico-w/pico_sdk_import.cmake)

# add_library(BUNDLE_LIBRARIES INTERFACE)
# target_link_libraries(BUNDLE_LIBRARIES INTERFACE )
project(${PROJECT_NAME} LANGUAGES CXX C ASM)

# initialize the Raspberry Pi Pico SDK
# after project!
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Config)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/e-Paper)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Fonts)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/GUI)

# ------ PLATFORM
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/platform/raspberry-pico-w)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/platform/raspberry-pico-w)

# ------ MIDDLEWARE
#set(MIDDLEWARE_SRCS library/st7789.c)

add_executable(${PROJECT_NAME} ${OST_SRCS} ${MIDDLEWARE_SRCS})

# ------ Link
target_link_libraries(${PROJECT_NAME} PUBLIC "-Wl,--start-group"
	gcc m c stdc++ raspberry-pico-w pico_stdlib hardware_spi hardware_dma hardware_pio ePaper GUI Fonts Config "-Wl,--end-group")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

pico_add_extra_outputs(${PROJECT_NAME})


# ==================================================================================================
# CUSTOM TARGETS
# ==================================================================================================
add_custom_target(RaspberryPico
    COMMAND ${CMAKE_COMMAND}
    -DOST_BUNDLE=RASPI_PICO
    -DPICO_SDK_PATH=../pico-sdk
    -DPICO_BOARD=pico_w
    -DCMAKE_BUILD_TYPE=Debug
    -B "${CMAKE_CURRENT_BINARY_DIR}/RaspberryPico"
    -S "${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_CURRENT_BINARY_DIR}/RaspberryPico"
)
