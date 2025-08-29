if(NOT DEFINED ELF OR NOT DEFINED NM)
  message(FATAL_ERROR "[garlic] Missing args for check_symbol.cmake")
endif()

message(STATUS "[garlic] Checking app thread is linked in ${ELF}")
execute_process(
  COMMAND ${NM} -n ${ELF}
  OUTPUT_VARIABLE NMOUT
  ERROR_QUIET
)
string(REGEX MATCH "app_thread|garlic_app_thread" MATCHED "${NMOUT}")
if(NOT MATCHED)
  message(FATAL_ERROR "[garlic] ERROR: app runtime thread symbol not found in ELF. Did weak main() run?")
endif()

