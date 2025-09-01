target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/app_runtime.c
    ${CMAKE_CURRENT_LIST_DIR}/src/uart_runtime.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_runtime.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)

# Notes for future maintainers:
# - Do NOT rely on overriding Zephyr's weak main(). Link order can cause the
#   kernel's main_weak to run instead of our app. We start the app via a
#   dedicated Zephyr thread (see app_runtime.c: K_THREAD_DEFINE), which is
#   robust to link-order changes.
# - Add app sources directly to the `app` target so they are always linked in.
#   (Avoid burying app entry points in zephyr_library-only modules.)
# - A post-link check in app/CMakeLists.txt verifies our runtime thread symbol
#   is present in the ELF, failing the build if it is missing.

# Keep app target non-empty for prelinking (harmless alongside real sources)
# No extra placeholders needed; app sources are defined explicitly above.
