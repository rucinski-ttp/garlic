# UART DMA driver built into the app target
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/uart_dma/src/uart_dma.c)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/uart_dma/inc
    ${CMAKE_CURRENT_LIST_DIR}/../utils/circular_buffer/inc
)
