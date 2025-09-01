cmake_minimum_required(VERSION 3.20)
project(garlic_unit_tests)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# FetchContent for Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_library(circular_buffer_host STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/utils/circular_buffer/src/circular_buffer.c
)

target_include_directories(circular_buffer_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)

# Proto libraries for host testing
add_library(proto_host STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/proto/crc32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/proto/transport.c
)

target_include_directories(proto_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)

add_library(commands_host STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/src/registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/src/register_all.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/git_version/src/git_version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/uptime/src/uptime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/flash_read/src/flash_read.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/stack/cmd_transport/src/cmd_transport.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/echo/src/echo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/i2c/src/i2c_cmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/tmp119/src/tmp119_cmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/drivers/tmp119/src/tmp119.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/commands/ble_ctrl/src/ble_ctrl.c
)

target_include_directories(commands_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)

add_executable(garlic_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_circular_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_build_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_crc32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_transport_encode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_transport_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_transport_edges.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command/test_command_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command/test_handlers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command/test_glue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command/test_glue_more.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tmp119_commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tmp119_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command/stubs/system_iface_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/command/stubs/reboot_stub.c
)

# Link Google Test
target_link_libraries(garlic_tests
    circular_buffer_host
    proto_host
    commands_host
    
)

target_include_directories(garlic_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)

target_link_libraries(garlic_tests
    GTest::gtest_main
    GTest::gtest
    GTest::gmock
)

# Link mocks for I2C async to satisfy TMP119/i2c commands in host tests
add_library(i2c_async_mock STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/i2c_async_mock.c
)
target_include_directories(i2c_async_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)
target_link_libraries(garlic_tests i2c_async_mock)

# UART DMA host build with test shim
add_library(uart_dma_host STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/drivers/uart_dma/src/uart_dma.c
)
target_compile_definitions(uart_dma_host PRIVATE UART_DMA_TESTING=1)
target_include_directories(uart_dma_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)
target_link_libraries(uart_dma_host PUBLIC circular_buffer_host)

target_link_libraries(garlic_tests uart_dma_host)

# Stubs for BLE runtime control symbols on host
add_library(ble_nus_stub STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/command/stubs/ble_nus_stub.c
)
target_include_directories(ble_nus_stub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)
target_link_libraries(commands_host PUBLIC ble_nus_stub)

# Stub for project_assert in unit tests
add_library(project_assert_stub STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/project_assert_stub.c
)
target_include_directories(project_assert_stub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)
target_link_libraries(garlic_tests project_assert_stub)

include(GoogleTest)

add_executable(garlic_tests_uartdma
    uart_dma/test_uart_dma.cpp
    uart_dma/test_uart_dma_tx.cpp
)
target_link_libraries(garlic_tests_uartdma
    uart_dma_host
    GTest::gtest_main
    GTest::gtest
    GTest::gmock
)
target_include_directories(garlic_tests_uartdma PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
)
gtest_discover_tests(garlic_tests_uartdma)

# Provide GARLIC_GIT_HASH to unit tests as in app build
execute_process(
  COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE UNIT_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if (NOT UNIT_GIT_HASH)
  set(UNIT_GIT_HASH "unknown")
endif()
target_compile_definitions(garlic_tests PRIVATE GARLIC_GIT_HASH="${UNIT_GIT_HASH}")

# Add test discovery
gtest_discover_tests(garlic_tests)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS garlic_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)
