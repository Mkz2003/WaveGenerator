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

# Set the project name
set(CMAKE_PROJECT_NAME JXM-AUDIO-SIGNAL-01-2026-07-TEST)

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
    # Add user sources here
    ${CMAKE_CURRENT_LIST_DIR}/USER/ADCFilter.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/WaveGenerate.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/TM1638.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/MyCode.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/Callback.c 
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_LIST_DIR}/USER/
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    ${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/Lib/GCC/libarm_cortexM0l_math.a
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # ---------- 编译选项 ----------
    add_compile_options(
        ${CPU_FLAGS}
        # 最高优化等级
        -O3
        # 不生成调试信息（逆向时失去符号、行号等线索）
        -g0
        # 隐藏所有符号的可见性，使导出符号更少
        -fvisibility=hidden
        # 每个函数/数据放入独立段，配合 gc-sections 移除未使用的代码
        -ffunction-sections
        -fdata-sections
        # 省略帧指针，进一步打乱栈回溯
        -fomit-frame-pointer
        # 不生成异常处理表（Cortex-M 通常不用）
        -fno-unwind-tables
        -fno-asynchronous-unwind-tables
        # 移除编译器版本标识
        -fno-ident
        # 启用链接时优化（跨文件内联、虚函数消除等，让代码边界更模糊）
        -flto
        # 可选：不使用栈保护（减少特征码）
        -fno-stack-protector
    )

    # ---------- 链接选项 ----------
    add_link_options(
        ${CPU_FLAGS}
        # 链接时剥离所有符号（生成的 elf 已经没有符号）
        -Wl,--strip-all
        # 配合 -ffunction-sections，丢弃未用到的段
        -Wl,--gc-sections
        # 不生成 build-id
        -Wl,--build-id=none
        # 同样启用 LTO
        -flto
        # 不输出 MAP 文件（或手动生成后保密，不要随固件发布）
        -Wl,-Map=/dev/null
    )

    # 假设你的固件目标名为 ${PROJECT_NAME}.elf
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # 1. 彻底 strip：移除所有符号、调试节、注释等
        COMMAND ${CMAKE_OBJCOPY} --strip-all 
                -R .comment -R .note -R .debug_frame 
                $<TARGET_FILE:${PROJECT_NAME}> 
                ${CMAKE_BINARY_DIR}/JXM-AUDIO-SIGNAL-01-2026-07-TEST.elf
        # 2. 从 stripped.elf 生成 bin 和 hex
        COMMAND ${CMAKE_OBJCOPY} -O binary 
                ${CMAKE_BINARY_DIR}/JXM-AUDIO-SIGNAL-01-2026-07-TEST.elf 
                ${CMAKE_BINARY_DIR}/JXM-AUDIO-SIGNAL-01-2026-07-TEST.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex 
                ${CMAKE_BINARY_DIR}/JXM-AUDIO-SIGNAL-01-2026-07-TEST.elf 
                ${CMAKE_BINARY_DIR}/JXM-AUDIO-SIGNAL-01-2026-07-TEST.hex
        COMMENT "Strip symbols and generate binaries for release"
    )

endif()
