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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src/utils/circular_buffer/inc
)

add_executable(garlic_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_circular_buffer.cpp
)

# Link Google Test
target_link_libraries(garlic_tests
    circular_buffer_host
    GTest::gtest_main
    GTest::gtest
    GTest::gmock
)

# Add test discovery
include(GoogleTest)
gtest_discover_tests(garlic_tests)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS garlic_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)
