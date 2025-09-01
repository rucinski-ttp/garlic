target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_ctrl.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)

