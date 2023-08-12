set( SOURCES
test_case.cc
parse.cc
rego_test.cc
)

add_executable(rego_test main.cc ${SOURCES})
target_link_libraries(rego_test
  PRIVATE 
  regocpp::rego)

add_executable(yaml_trieste driver.cc ${SOURCES})

target_link_libraries(yaml_trieste
  PRIVATE
  regocpp::rego)

add_test(NAME rego_trieste COMMAND rego_trieste test -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_trieste>)
add_test(NAME yaml_trieste COMMAND yaml_trieste test -f WORKING_DIRECTORY $<TARGET_FILE_DIR:yaml_trieste>)
add_test(NAME rego_test COMMAND rego_test regocpp.yaml WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_test>)

add_custom_command(TARGET rego_test POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/regocpp.yaml $<TARGET_FILE_DIR:rego_test>/regocpp.yaml)
