target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/crc32.c
    ${CMAKE_CURRENT_LIST_DIR}/transport.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)
