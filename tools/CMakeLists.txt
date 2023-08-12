
add_executable(rego_interpreter main.cc)
target_link_libraries(rego_interpreter
  PRIVATE 
  regocpp::rego)

add_executable(rego_trieste driver.cc)

target_link_libraries(rego_trieste
  PRIVATE
  regocpp::rego)

add_custom_command(TARGET rego_interpreter POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples $<TARGET_FILE_DIR:rego_interpreter>/examples)
