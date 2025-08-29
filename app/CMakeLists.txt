cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(garlic)

# Include directories
target_include_directories(app PRIVATE 
    include
)

# Source files organized by module
# For debugging bring-up, you can switch between apps here
target_sources(app PRIVATE 
    # Application - UART DMA echo + status
    src/app/main.c

    # Drivers
    src/drivers/uart_dma.c

    # Utilities
    src/utils/circular_buffer.c
)
