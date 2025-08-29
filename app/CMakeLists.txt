cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(garlic)

# Include directories for modular libs used by main
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

# Source files organized by module
# For debugging bring-up, you can switch between apps here
add_subdirectory(src)
add_subdirectory(src/app)
 

# Embed current git hash into the firmware for RTT boot print
execute_process(
  COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GARLIC_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if (NOT GARLIC_GIT_HASH)
  set(GARLIC_GIT_HASH "unknown")
endif()
message(STATUS "Garlic git hash: ${GARLIC_GIT_HASH}")
target_compile_definitions(app PRIVATE GARLIC_GIT_HASH="${GARLIC_GIT_HASH}")

# Also make git hash visible to all zephyr_library() units
zephyr_compile_definitions(GARLIC_GIT_HASH="${GARLIC_GIT_HASH}")


# Also write the git hash to the build directory so integration tests can read it
file(WRITE ${CMAKE_BINARY_DIR}/garlic_git_hash.txt "${GARLIC_GIT_HASH}\n")
# Extra compiler warnings
zephyr_compile_options(-Wparentheses)

# -------------------------------
# Post-link guardrail: verify our app thread is linked
# -------------------------------
# This catches regressions where Zephyr's weak main() runs and our runtime
# thread is not present (e.g., due to link order or misplaced sources).
set(NM_FALLBACK nm)
if(DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
  set(NM_CAND "$ENV{ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-nm")
  if(EXISTS ${NM_CAND})
    set(NM_FALLBACK ${NM_CAND})
  endif()
endif()

add_custom_target(garlic_post_link ALL
  COMMAND ${CMAKE_COMMAND} -D ELF=${CMAKE_BINARY_DIR}/zephyr/zephyr.elf -D NM=${NM_FALLBACK} -P ${CMAKE_CURRENT_LIST_DIR}/cmake/check_symbol.cmake
  DEPENDS zephyr_final
  COMMENT "Post-link: verify runtime thread symbol present"
)

# -------------------------------
# clang-format targets (format / format-check)
# -------------------------------
find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-19 clang-format-18 clang-format-17)
if(CLANG_FORMAT_BIN)
  # Collect app source files for formatting
  file(GLOB_RECURSE APP_FORMAT_FILES
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    src/*.c src/*.h src/*.cpp src/*.hpp
  )
  list(FILTER APP_FORMAT_FILES INCLUDE REGEX "^src/.*\\.(c|h|cpp|hpp)$")

  add_custom_target(format
    COMMAND ${CMAKE_COMMAND}
      -DCLANG_FORMAT_PROG=${CLANG_FORMAT_BIN}
      -DCWD=${CMAKE_CURRENT_LIST_DIR}
      -DFILES="${APP_FORMAT_FILES}"
      -DCHECK=0
      -P ${CMAKE_CURRENT_LIST_DIR}/cmake/run_clang_format.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Running clang-format (in-place) on app sources"
  )

  add_custom_target(format-check
    COMMAND ${CMAKE_COMMAND}
      -DCLANG_FORMAT_PROG=${CLANG_FORMAT_BIN}
      -DCWD=${CMAKE_CURRENT_LIST_DIR}
      -DFILES="${APP_FORMAT_FILES}"
      -DCHECK=1
      -P ${CMAKE_CURRENT_LIST_DIR}/cmake/run_clang_format.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Running clang-format --dry-run on app sources"
  )
else()
  message(STATUS "clang-format not found; format targets disabled")
endif()
