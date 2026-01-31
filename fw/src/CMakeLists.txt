cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)
set(PROJECT_NAME picoclock)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

set(PICO_BOARD pico2_picoclock)
#set(PICO_BOARD pico2_w)

# ==========================
# DEFAULT VARIABLES OPTIONS
# ==========================
if(NOT OST_BUNDLE)
    set(OST_BUNDLE "RASPI_PICO")
endif()

if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "../lib/pico-sdk")
endif()

if(NOT PICO_EXTRAS_PATH)
    set(PICO_EXTRAS_PATH "../lib/pico-extras")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


# ==========================
# COMMON FILES
# ==========================
set(OST_SRCS
    system/main.c
    system/alarms.c
    system/utils.c
    system/sdcard.c
    system/ff_diskio_sdcard.c
    system/debug.c
    system/filesystem.c
    system/sdcard.c
    system/audio_player.c
    system/ui_task.c
    system/net_task.c
    system/fs_task.c
    system/audio_player.c
    system/ff/ff.c
    system/ff/ffsystem.c
    system/ff/ff_stubs.c
    library/mini_qoi.c
    library/tiny-json.c
    library/ens160/ens160.c
    library/mcp9808/mcp9808.c
    library/pcf8563/pcf8563.c
    library/mcp46XX/mcp45XX.c
    library/lp5817/lp5817.c
    library/circularBuffer.c
    library/graphics.c
    library/tinyhttp/http.c
    library/tinyhttp/chunk.c
    library/tinyhttp/header.c
    library/flash_storage.c
    library/pico_i2s.c
    library/ini.c
    core/wifi.c
)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library
    ${CMAKE_CURRENT_SOURCE_DIR}/system/ff
    ${CMAKE_CURRENT_SOURCE_DIR}/system/
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/e-Paper
    ${CMAKE_CURRENT_SOURCE_DIR}/core/
)

message("=============  TARGET: RASPBERRY PICO  =============")

add_definitions(-DOST_USE_FF_LIBRARY)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(${CMAKE_CURRENT_SOURCE_DIR}/core/pico_sdk_import.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/core/pico_extras_import.cmake)

# add_library(BUNDLE_LIBRARIES INTERFACE)
# target_link_libraries(BUNDLE_LIBRARIES INTERFACE )
project(${PROJECT_NAME} LANGUAGES CXX C ASM)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Config)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/e-Paper)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/Fonts)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/library/epaper/GUI)

# ------ Executable
add_executable(${PROJECT_NAME} ${OST_SRCS} ${MIDDLEWARE_SRCS})

# ------ Link
target_link_libraries(${PROJECT_NAME} PUBLIC "-Wl,--start-group"
	gcc m c stdc++ pico_stdlib pico_util pico_aon_timer pico_multicore
	hardware_spi hardware_dma hardware_pio hardware_i2c hardware_exception
	cmsis_core
	pico_cyw43_arch_lwip_poll
	#pico_cyw43_arch_lwip_threadsafe_background
	ePaper GUI Fonts Config pico_audio_i2s "-Wl,--end-group")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

pico_add_extra_outputs(${PROJECT_NAME})


pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/library/pico_i2s.pio)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)


target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    SERVER_IP=\"${SERVER_IP}\"
    SERVER_PORT=\"${SERVER_PORT}\"
    PICO_AUDIO_I2S_MONO_INPUT=1
)

# ==========================
# CUSTOM TARGETS
# ==========================
add_custom_target(RaspberryPico
    COMMAND ${CMAKE_COMMAND}
    -DOST_BUNDLE=RASPI_PICO
    -DCMAKE_BUILD_TYPE=Debug
    -B "${CMAKE_CURRENT_BINARY_DIR}/RaspberryPico"
    -S "${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_CURRENT_BINARY_DIR}/RaspberryPico"
)
