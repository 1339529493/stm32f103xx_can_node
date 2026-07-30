cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(APP_TAG_PROTOCALS app bootloader)
set(APP_TAG app CACHE STRING "")
set_property(CACHE APP_TAG PROPERTY STRINGS ${APP_TAG_PROTOCALS})

if(APP_TAG STREQUAL "app")
aux_source_directory(app/src MIAN_APP_SRCS)
list(APPEND MIAN_APP_INCLUDES 
    app/src
)
set(LINK_PATH app/linker)
list(APPEND ALL_BUILD_SRCS
    ${MIAN_APP_SRCS}
)
list(APPEND ALL_BUILD_INCLUDES 
    ${MIAN_APP_INCLUDES}
)
# list(APPEND ALL_BUILD_DEFINE 
#     VECT_TAB_OFFSET=0x4000
# )
elseif(APP_TAG STREQUAL "bootloader")
aux_source_directory(bootloader/src BOOTLOADER_APP_SRCS)
aux_source_directory(bootloader/src/can CAN_APP_SRCS)
list(APPEND BOOTLOADER_APP_INCLUDES 
    bootloader/src
    bootloader/src/can
)
set(LINK_PATH bootloader/linker)
list(APPEND ALL_BUILD_SRCS
    ${BOOTLOADER_APP_SRCS}
    ${CAN_APP_SRCS}
)
list(APPEND ALL_BUILD_INCLUDES 
    ${BOOTLOADER_APP_INCLUDES}
)
endif()

message("Build target: ${APP_TAG}")

# Set the project name
set(CMAKE_PROJECT_NAME ${APP_TAG})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_BUILD_SRCS}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_BUILD_INCLUDES}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_BUILD_DEFINE}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# 生成 .bin 和 .hex 文件
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary 
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}> 
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex 
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}> 
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} 
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating firmware files (.bin, .hex)"
)

# 可选：复制到特定目录
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin 
            ${CMAKE_CURRENT_BINARY_DIR}/firmware/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Copying .bin to firmware directory"
)
