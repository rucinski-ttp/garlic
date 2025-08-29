zephyr_library()
zephyr_library_sources(uart_dma/src/uart_dma.c)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/uart_dma/inc
    ${CMAKE_CURRENT_LIST_DIR}/../utils/circular_buffer/inc
)
