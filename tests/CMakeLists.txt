add_executable(rego_test main.cc test_case.cc)
target_link_libraries(rego_test
  PRIVATE 
  regocpp::rego)

add_executable(rego_test_c_api c_api.cc)
target_link_libraries(rego_test_c_api
  PRIVATE 
  regocpp::rego)

if(REGOCPP_BUILD_TOOLS)
  add_test(NAME rego_fuzzer_reader COMMAND rego_fuzzer reader -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
  add_test(NAME rego_fuzzer_unify COMMAND rego_fuzzer unify -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
endif()

add_test(NAME rego_invalid_input COMMAND rego -i bad.json data WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
add_test(NAME rego_invalid_large COMMAND rego -d rego data WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
add_test(NAME rego_test_regocpp COMMAND rego_test regocpp.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_bugs COMMAND rego_test bugs.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_v1 COMMAND rego_test regov1.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_aci COMMAND rego_test aci/aci.yaml WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_c_api COMMAND rego_test_c_api WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
set_property(TEST rego_invalid_input PROPERTY WILL_FAIL On)
set_property(TEST rego_invalid_large PROPERTY WILL_FAIL On)
set_property(TEST rego_test_aci PROPERTY TIMEOUT 300)

add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/regocpp.yaml $<TARGET_FILE_DIR:rego_test>/regocpp.yaml)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/regov1.yaml $<TARGET_FILE_DIR:rego_test>/regov1.yaml)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bugs.yaml $<TARGET_FILE_DIR:rego_test>/bugs.yaml)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bad.json $<TARGET_FILE_DIR:rego_interpreter>/bad.json)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/aci $<TARGET_FILE_DIR:rego_test>/aci)

if(REGOCPP_OPA_TESTS)
  set( OPA_TEST_DIRS
  aggregates
  all
  any
  arithmetic
  array
  assignments
  base64builtins
  base64urlbuiltins
  baseandvirtualdocs
  bitsand
  bitsnegate
  bitsor
  bitsshiftleft
  bitsshiftright
  bitsxor
  casts
  comparisonexpr
  completedoc
  compositebasedereference
  compositereferences
  comprehensions
  containskeyword
  dataderef
  defaultkeyword
  disjunction
  elsekeyword
  embeddedvirtualdoc
  eqexpr
  evaltermexpr
  every
  example
  fix1863
  functionerrors
  functions
  helloworld
  hexbuiltins
  indexing
  indirectreferences
  inputvalues
  intersection
  jsonbuiltins
  negation
  nestedreferences
  numbersrange
  numbersrangestep
  objectfilter
  objectfilteridempotent
  objectfilternonstringkey
  objectget
  objectkeys
  objectremove
  objectremoveidempotent
  objectremovenonstringkey
  objectunion
  objectunionn
  partialdocconstants
  partialiter
  partialobjectdoc
  partialsetdoc
  rand
  reachable
  refheads
  regexfind
  regexfindallstringsubmatch
  regexisvalid
  regexmatch
  regexreplace
  regexsplit
  regexmatchtemplate
  replacen
  semvercompare
  semverisvalid
  sets
  sprintf
  strings
  subset
  toarray
  topdowndynamicdispatch
  toset
  trim
  trimleft
  trimprefix
  trimright
  trimspace
  trimsuffix
  type
  typebuiltin
  typenamebuiltin
  undos
  union
  units
  urlbuiltins
  varreferences
  virtualdocs
  withkeyword
  )

  set( OPA_TEST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/../opa/test/cases/testdata )
  foreach( TEST_DIR ${OPA_TEST_DIRS})
    add_test(NAME rego_test_${TEST_DIR}
             COMMAND rego_test -wf ${OPA_TEST_ROOT}/${TEST_DIR}/
             WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
  endforeach()
endif()
