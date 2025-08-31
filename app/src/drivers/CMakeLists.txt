# UART DMA driver built into the app target
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/uart_dma/src/uart_dma.c)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/uart_dma/inc
    ${CMAKE_CURRENT_LIST_DIR}/../utils/circular_buffer/inc
)

# I2C async helper
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/i2c_async/src/i2c_async.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/i2c_async/inc)

# TMP119 sensor driver
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tmp119/src/tmp119.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tmp119/inc)
