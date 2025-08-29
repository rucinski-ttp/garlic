target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/registry.c
    ${CMAKE_CURRENT_LIST_DIR}/src/register_all.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)

# Submodules (each command and glue provides its own library parts)
add_subdirectory(git_version)
add_subdirectory(uptime)
add_subdirectory(flash_read)
add_subdirectory(reboot)
add_subdirectory(core)
add_subdirectory(echo)
add_subdirectory(i2c)
add_subdirectory(tmp119)
