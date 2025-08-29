add_library(uart_dma STATIC
    src/uart_dma.c
)

target_include_directories(uart_dma
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(uart_dma
    PUBLIC
        circular_buffer
)

