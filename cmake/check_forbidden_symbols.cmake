set(FORBIDDEN_SYMBOLS
  __cxa_thread_atexit_impl
)

execute_process(
  COMMAND "${NM}" -uD "${SHARED_LIB}"
  OUTPUT_VARIABLE nm_output
  RESULT_VARIABLE nm_result
  ERROR_VARIABLE nm_error
)

if(NOT nm_result EQUAL 0)
  message(FATAL_ERROR
    "check_forbidden_symbols: '${NM} -uD ${SHARED_LIB}' failed "
    "(exit ${nm_result}): ${nm_error}")
endif()

string(REPLACE "\n" ";" nm_lines "${nm_output}")

set(found_forbidden "")
foreach(line IN LISTS nm_lines)
  foreach(sym IN LISTS FORBIDDEN_SYMBOLS)
    if(line MATCHES "^[ \t]+U[ \t]+${sym}$")
      list(APPEND found_forbidden "${sym}")
    endif()
  endforeach()
endforeach()
list(REMOVE_DUPLICATES found_forbidden)

if(found_forbidden)
  message(FATAL_ERROR
    "\n"
    "${SHARED_LIB} references forbidden undefined symbols:\n"
    "    ${found_forbidden}\n"
    "\n"
    "These symbols are provided by glibc but not by musl, so the library "
    "would fail to load on Alpine. Check the SNMALLOC_CLEANUP override in "
    "the root CMakeLists.txt and any new dependency for calls into "
    "glibc-only interfaces.\n"
  )
endif()
