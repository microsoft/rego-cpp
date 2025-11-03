add_executable(rego_test main.cc test_case.cc builtins.cc)
target_link_libraries(rego_test
  PRIVATE 
  regocpp::rego)

add_executable(rego_test_c_api c_api.cc)
target_link_libraries(rego_test_c_api
  PRIVATE 
  regocpp::rego)

add_executable(rego_test_cpp_api cpp_api.cc)
target_link_libraries(rego_test_cpp_api
  PRIVATE 
  regocpp::rego)


if(REGOCPP_BUILD_TOOLS)
  add_test(NAME rego_fuzzer_file_to_rego COMMAND rego_fuzzer file_to_rego -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
  add_test(NAME rego_fuzzer_rego_to_bundle COMMAND rego_fuzzer rego_to_bundle -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
  add_test(NAME rego_fuzzer_json_to_bundle COMMAND rego_fuzzer json_to_bundle -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
  add_test(NAME rego_fuzzer_bundle_to_json COMMAND rego_fuzzer bundle_to_json -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_fuzzer>)
endif()

add_test(NAME rego_invalid_input COMMAND rego -i bad.json data WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
add_test(NAME rego_invalid_large COMMAND rego -d rego data WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
add_test(NAME rego_test_manual COMMAND rego_test -n manual WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
add_test(NAME rego_test_regocpp COMMAND rego_test regocpp.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_bundle_json COMMAND rego_test regocpp.yaml -r json -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_bundle_binary COMMAND rego_test regocpp.yaml -r binary -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_bugs COMMAND rego_test bugs.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_cts COMMAND rego_test cts/cts.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_aci COMMAND rego_test aci/aci.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_c_api COMMAND rego_test_c_api WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
add_test(NAME rego_test_cpp_api COMMAND rego_test_cpp_api WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
set_property(TEST rego_invalid_input PROPERTY WILL_FAIL On)
set_property(TEST rego_invalid_large PROPERTY WILL_FAIL On)
set_property(TEST rego_test_aci PROPERTY TIMEOUT 300)

if(MSVC)
  message("cheriot tests are disabled on Windows due to lack of demangling support")
else()
  add_test(NAME rego_test_cheriot COMMAND rego_test cheriot/cheriot.yaml -wf WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
endif()


add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/regocpp.yaml $<TARGET_FILE_DIR:rego_test>/regocpp.yaml)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bugs.yaml $<TARGET_FILE_DIR:rego_test>/bugs.yaml)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bad.json $<TARGET_FILE_DIR:rego_interpreter>/bad.json)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/aci $<TARGET_FILE_DIR:rego_test>/aci)
add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/cts $<TARGET_FILE_DIR:rego_test>/cts)
add_custom_command(TARGET rego_test POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/cheriot $<TARGET_FILE_DIR:rego_test>/cheriot)
add_custom_command(TARGET rego_test POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../tools/examples $<TARGET_FILE_DIR:rego_test>/examples)


if(REGOCPP_OPA_TESTS)
  set( OPA_TEST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/../opa/v1/test/cases/testdata/v1 )
  add_test(NAME rego_test_opa
            COMMAND rego_test -wf ${OPA_TEST_ROOT}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
  set_property(TEST rego_test_opa PROPERTY TIMEOUT 300)
  if(REGOCPP_OPA_ROUNDTRIP_TESTS)
    add_test(NAME rego_test_opa_bundle_json
            COMMAND rego_test -wf -r json ${OPA_TEST_ROOT}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
    set_property(TEST rego_test_opa_bundle_json PROPERTY TIMEOUT 300)
    add_test(NAME rego_test_opa_bundle_binary
            COMMAND rego_test -wf -r binary ${OPA_TEST_ROOT}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)
     set_property(TEST rego_test_opa_bundle_binary PROPERTY TIMEOUT 300)
  endif()
endif()
