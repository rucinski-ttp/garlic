if(NOT DEFINED CLANG_FORMAT_PROG)
  message(FATAL_ERROR "CLANG_FORMAT_PROG not set")
endif()
if(NOT DEFINED FILES)
  message(FATAL_ERROR "FILES not set")
endif()
if(NOT DEFINED CWD)
  set(CWD ${CMAKE_CURRENT_LIST_DIR})
endif()

set(_opts -i)
if(CHECK)
  set(_opts --dry-run --Werror)
endif()

set(_failed FALSE)
# Ensure FILES is a proper CMake list (semicolon-separated)
set(_FILES_LIST "${FILES}")
string(REPLACE " " ";" _FILES_LIST "${_FILES_LIST}")
foreach(f IN LISTS _FILES_LIST)
  execute_process(
    COMMAND ${CLANG_FORMAT_PROG} ${_opts} ${f}
    WORKING_DIRECTORY ${CWD}
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(STATUS "needs formatting: ${f}")
    set(_failed TRUE)
  endif()
endforeach()

if(CHECK AND _failed)
  message(FATAL_ERROR "clang-format check failed")
endif()
