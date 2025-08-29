# Circular buffer module (compiled into app target)
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/circular_buffer.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)
