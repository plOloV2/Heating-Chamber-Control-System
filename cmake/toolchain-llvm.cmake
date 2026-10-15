set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)

# arm-none-eabi- prefix
set(TOOLCHAIN_PREFIX                arm-)

# Core MCU flags, CPU type, instruction set and FPU setup
set(CPU_PARAMS                      -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

# C comon flags
set(FLAGS                           --target=arm-none-eabi -fdata-sections -ffunction-sections -pipe)

# Error flags
set(ERROR_FLAGS                     -Wall -Wextra -Werror)

# Linker flags
set(LINKER_FLAGS                    -fuse-ld=lld -Wl,--gc-sections,-Map=${CMAKE_PROJECT_NAME}.map,--print-memory-usage -nostartfiles)

# Define compiler settings
set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}clang ${FLAGS})
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
find_program(CMAKE_OBJCOPY          NAMES ${TOOLCHAIN_PREFIX}llvm-objcopy HINTS /usr/local/bin REQUIRED)
find_program(CMAKE_SIZE             NAMES ${TOOLCHAIN_PREFIX}llvm-size    HINTS /usr/local/bin REQUIRED)

# This will create .elf files
set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")
