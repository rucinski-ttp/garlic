cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(garlic)

# Include directories for modular libs used by main
zephyr_include_directories(
    src/drivers/uart_dma/inc
    src/utils/circular_buffer/inc
)

# Source files organized by module
# For debugging bring-up, you can switch between apps here
add_subdirectory(src)

# Application entry
target_sources(app PRIVATE src/app/main.c)

# Pull in module sources directly for application
target_sources(app PRIVATE
    src/drivers/uart_dma/src/uart_dma.c
    src/utils/circular_buffer/src/circular_buffer.c
)

# Embed current git hash into the firmware for RTT boot print
execute_process(
  COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GARLIC_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if (NOT GARLIC_GIT_HASH)
  set(GARLIC_GIT_HASH "unknown")
endif()
message(STATUS "Garlic git hash: ${GARLIC_GIT_HASH}")
target_compile_definitions(app PRIVATE GARLIC_GIT_HASH="${GARLIC_GIT_HASH}")

# Also write the git hash to the build directory so integration tests can read it
file(WRITE ${CMAKE_BINARY_DIR}/garlic_git_hash.txt "${GARLIC_GIT_HASH}\n")
