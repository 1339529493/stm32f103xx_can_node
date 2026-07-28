set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
# arm-none-eabi- must be part of path environment
# # 使用 find_program 查找，不需要 .exe 后缀
# find_program(CC NAMES arm-none-eabi-gcc   PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)
# find_program(CXX NAMES arm-none-eabi-g++   PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)
# find_program(ASM NAMES arm-none-eabi-gcc   PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)
# find_program(LINK NAMES arm-none-eabi-g++   PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)
# find_program(OBJCOPY NAMES arm-none-eabi-objcopy PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)
# find_program(SIZE NAMES arm-none-eabi-size  PATHS ${TOOLCHAIN_PATH} REQUIRED NO_DEFAULT_PATH)

# # 标记为缓存变量
# set(CMAKE_C_COMPILER     ${CC}      CACHE FILEPATH "C compiler")
# set(CMAKE_CXX_COMPILER   ${CXX}     CACHE FILEPATH "C++ compiler")
# set(CMAKE_ASM_COMPILER   ${ASM}     CACHE FILEPATH "ASM compiler")
# set(CMAKE_LINKER         ${LINK}    CACHE FILEPATH "Linker")
# set(CMAKE_OBJCOPY        ${OBJCOPY} CACHE FILEPATH "Object copy")
# set(CMAKE_SIZE           ${SIZE}    CACHE FILEPATH "Size")

if(WIN32)
    set(TOOLCHAIN_SUFFIX ".exe")
else()
    set(TOOLCHAIN_SUFFIX "")
endif()
set(TOOLCHAIN_PREFIX ${TOOLCHAIN_PATH}/arm-none-eabi- CACHE STRING "Toolchain prefix")
set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_SUFFIX})
set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_SUFFIX})
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy${TOOLCHAIN_SUFFIX})
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size${TOOLCHAIN_SUFFIX})

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m3 ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections -fstack-usage")

# The cyclomatic-complexity parameter must be defined for the Cyclomatic complexity feature in STM32CubeIDE to work.
# However, most GCC toolchains do not support this option, which causes a compilation error; for this reason, the feature is disabled by default.
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcyclomatic-complexity")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/${LINK_PATH}/STM32F103xx_${APP_TAG}.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
set(TOOLCHAIN_LINK_LIBRARIES "m")
