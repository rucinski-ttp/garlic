# UART driver built into the app target (renamed from uart_dma)
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/uart/src/uart_dma.c)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/uart/inc
    ${CMAKE_CURRENT_LIST_DIR}/../utils/circular_buffer/inc
)

# I2C async helper (renamed from i2c_async)
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/i2c/src/i2c.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/i2c/inc)

# TMP119 sensor driver
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tmp119/src/tmp119.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tmp119/inc)

# BLE NUS driver
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ble_nus/src/ble_nus.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ble_nus/inc)
