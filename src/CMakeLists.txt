add_library(rego
  lang.cc
  math.cc
  parse.cc
  input_data.cc
  modules.cc
  lists.cc
  structure.cc
  strings.cc
  symbols.cc
  multiply_divide.cc
  add_subtract.cc
  comparison.cc
  merge_data.cc
  merge_modules.cc
  rules_engine.cc
  rules.cc
  query.cc
)

target_link_libraries(rego
  snmallocshim-static
  re2::re2
  CLI11::CLI11
  trieste::trieste
)

add_executable(rego_interpreter main.cc)
target_link_libraries(rego_interpreter rego)

add_executable(rego_trieste driver.cc)

target_link_libraries(rego_trieste rego)

set(tests_list
scalars "-d\texamples/scalars.rego\t-q\tdata.scalars.greeting" "\"Hello\""
objects "-d\texamples/objects.rego\t-q\tdata.objects.sites[1].name" "\"smoke1\""
input_data "-d\texamples/data0.json\texamples/data1.json\texamples/objects.rego\t-i\texamples/input0.json\t-q\t[data.one, input.b, data.objects.sites[1]]" [=[\[{"bar": "Foo", "baz": 5, "be": true, "bop": 23.4}, "20", {"name": "smoke1"}\]]=]
expressions "-d\texamples/expressions.rego\t-q\tdata.expressions.k > 27" "true"
query_math "-q\t5 + (2 - 4 * 0.25) * -3 + 7.4" "9.4"
bodies "-d\texamples/bodies.rego\t-i\texamples/input0.json\t-q\tdata.bodies.e" [=[{"one": 15, "two": 15}]=]
bodies_undef "-d\texamples/bodies.rego\t-i\texamples/input0.json\t-q\tdata.bodies.g" "undefined"
sets "-d\texamples/sets.rego\t-q\tdata.sets.string_set" [=[\["four", "two", "one"\]]=]
sets_membership "-d\texamples/sets.rego\t-q\tdata.sets.m" "true"
var_keys "-d\texamples/objects.rego\t-q\tdata.objects.e[\"dev\"]" "false"
multi0 "-d\texamples/multi.rego\t-i\texamples/input0.json\t-q\tdata.multi.a" "1"
multi1 "-d\texamples/multi.rego\t-i\texamples/input1.json\t-q\tdata.multi.a" "41"
multi2 "-d\texamples/multi.rego\t-i\texamples/input2.json\t-q\tdata.multi.a" "70"
)

add_test(NAME rego_trieste COMMAND rego_trieste test -f)

list(LENGTH tests_list tests_length)
while(${tests_length} GREATER 0)
  list(POP_FRONT tests_list test_name test_args test_regex)
  string(REPLACE "\t" ";" test_args_list ${test_args})
  add_test(NAME ${test_name}
           COMMAND rego_interpreter ${test_args_list}
           WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
  set_tests_properties(${test_name} PROPERTIES
           PASS_REGULAR_EXPRESSION "^${test_regex}")
  list(LENGTH tests_list tests_length)
endwhile()

add_custom_command(TARGET rego_interpreter POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples $<TARGET_FILE_DIR:rego_interpreter>/examples)
