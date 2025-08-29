target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/flash_read.c)
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/../inc
)
