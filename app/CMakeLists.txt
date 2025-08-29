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
