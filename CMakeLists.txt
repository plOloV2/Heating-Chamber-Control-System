# @author Aleksander Dziągwa

cmake_minimum_required(VERSION 4.1)

project(aero_oven C)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Setup C compiler settings
set(CMAKE_C_STANDARD                23)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)

# Sources
set(sources_SRCS
    app/src/main.c

    app/src/EVE4_Riverti_lib/screen_platform.c
    app/src/EVE4_Riverti_lib/App_Common.c
    app/src/EVE4_Riverti_lib/CoPro_Cmds.c
    app/src/EVE4_Riverti_lib/Hal_Utils.c
    app/src/EVE4_Riverti_lib/Gpu_Hal.c
    
    app/src/RTOS/temp_task.c
    app/src/RTOS/buzzer_task.c
    app/src/RTOS/create_tasks.c
    app/src/RTOS/SD_task.c

    startup/startup_stm32f405.c

    platform/pin_init.c
    platform/RTC_init.c
    platform/clock_init.c
    platform/clock_info.c
    platform/system_init.c
    platform/pin_operations.c
    platform/RTC_operations.c
    platform/buzzer_operations.c

    middlewares/FreeRTOS/vApplicationStackOverflowHook.c
    middlewares/FatFs/source/diskio.c
    middlewares/FatFs/source/ff.c
    middlewares/FatFs/source/ffsystem.c
    middlewares/FatFs/source/ffunicode.c
)

# Include directories
set(include_DIRS
    app/inc
    CMSIS/Core/Include
    CMSIS/Device/Include
    middlewares/FatFs/include
)

# Linker script
set(linker_script_SRC
    ${CMAKE_CURRENT_LIST_DIR}/linker/stm32f405rg_flash.ld
)

# App version check
add_custom_target(
    GenerateVersionHeader
    COMMAND ${CMAKE_COMMAND} 
        -D PROJ_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -D TEMPLATE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_version/version.h.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/app/inc/version.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_version/git_version.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/version.h
    COMMENT "Checking Git version..."
    VERBATIM
)


# Add global compilation flags
set(GLOBAL_FLAG_C
    -fstack-usage
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
)

# Set config flags
set(CMAKE_C_FLAGS_DEBUG             "-Og -ggdb3")
set(CMAKE_C_FLAGS_RELEASE           "-O3 -DNDEBUG -flto=auto")

# Set global flags
add_compile_options(${CPU_PARAMS} ${ERROR_FLAGS} ${GLOBAL_FLAG_C})

include(FetchContent)

# FreeRTOS config
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/middlewares/FreeRTOS)
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS Port" FORCE)

# Fetch FreeRTOS
FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG        V11.3.1  # Locked to a V11.3.1 version
)
FetchContent_MakeAvailable(freertos_kernel)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME} ${sources_SRCS})

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${include_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR} 
)

# Ensures the version header is generated before compilation
add_dependencies(${CMAKE_PROJECT_NAME} GenerateVersionHeader)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F405xx
    HSE_VALUE=16000000UL
    LSE_VALUE=32768UL
)

# Linker options
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -T${linker_script_SRC}
    ${CPU_PARAMS}
    ${LINKER_FLAGS}
)

# Link against FreeRTOS 
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE freertos_kernel)

# Set linker script as a dependency to force re-link if it changes
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_DEPENDS ${linker_script_SRC})

# Execute post-build to print size, generate hex and bin
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
)
