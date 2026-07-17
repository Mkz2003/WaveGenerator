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
set(CMAKE_PROJECT_NAME WaveGenerator)

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
    ${CMAKE_CURRENT_LIST_DIR}/USER/WaveGenerate.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/TM1638.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/MyCode.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/io.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/Callback.c
    ${CMAKE_CURRENT_LIST_DIR}/USER/Date.c    
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

    ${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM0l_math.a
)

# target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
#     -u _printf_float
# )

# 1. 列出所有需要强制编译的源文件（相对于项目根目录）
set(FORCE_COMPILE_SOURCES
    # 可继续添加更多文件
    ${CMAKE_CURRENT_LIST_DIR}/USER/Date.c
    
)

# 2. 生成所有 touch 命令
set(TOUCH_COMMANDS "")
foreach(src ${FORCE_COMPILE_SOURCES})
    list(APPEND TOUCH_COMMANDS COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${src})
endforeach()

# 3. 创建自定义目标，执行所有 touch 命令
add_custom_target(ForceCompileSources
    ${TOUCH_COMMANDS}
    COMMENT "Forcing recompilation of specified source files"
)

# 4. 将自定义目标添加为主目标的依赖
add_dependencies(${CMAKE_PROJECT_NAME} ForceCompileSources)
