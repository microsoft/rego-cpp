
add_executable(rego_interpreter main.cc)
target_link_libraries(rego_interpreter
  PRIVATE 
  regocpp::rego)

add_executable(rego_trieste driver.cc)

target_link_libraries(rego_trieste
  PRIVATE
  regocpp::rego)

set(tests_list
scalars "-d\texamples/scalars.rego\t-q\tdata.scalars.greeting" "\"Hello\""
objects "-d\texamples/objects.rego\t-q\tdata.objects.sites[1].name" "\"smoke1\""
input_data "-d\texamples/data0.json\texamples/data1.json\texamples/objects.rego\t-i\texamples/input0.json\t-q\t[data.one, input.b, data.objects.sites[1]]" [=[\[{"bar": "Foo", "baz": 5, "be": true, "bop": 23.4}, "20", {"name": "smoke1"}\]]=]
expressions "-d\texamples/expressions.rego\t-q\tdata.expressions.k > 27" "true"
bodies "-d\texamples/bodies.rego\t-i\texamples/input0.json\t-q\tdata.bodies.e" [=[{"one": 15, "two": 15}]=]
bodies_undef "-d\texamples/bodies.rego\t-i\texamples/input0.json\t-q\tdata.bodies.g" "undefined"
sets "-d\texamples/sets.rego\t-q\tdata.sets.string_set" [=[\["four", "one", "two"\]]=]
sets_membership "-d\texamples/sets.rego\t-q\tdata.sets.m" "true"
var_keys "-d\texamples/objects.rego\t-q\tdata.objects.f" "false"
multi0 "-d\texamples/multi.rego\t-i\texamples/input0.json\t-q\tdata.multi.a" "1"
multi1 "-d\texamples/multi.rego\t-i\texamples/input1.json\t-q\tdata.multi.a" "41"
multi2 "-d\texamples/multi.rego\t-i\texamples/input2.json\t-q\tdata.multi.a" "70"
functions_sum "-d\texamples/functions.rego\t-i\texamples/input1.json\t-q\t[data.functions.f(1, 2, 3), data.functions.f(1, -1, 3)]" [=[\[6, 4\]]=]
functions_prod "-d\texamples/functions.rego\t-i\texamples/input2.json\t-q\t[data.functions.f(1, 2, 3), data.functions.f(1, -1, 3)]" [=[\[6, -3\]]=]
functions_ref "-d\texamples/functions.rego\t-q\t[data.functions.h(1, 2).a, data.functions.h(3, 4).b]" [=[\[1, 4\]]=]
unify "-d\texamples/unify.rego\t-q\tdata.unify.output" [=[\[{"bar": 2, "foo": 1}, 3, true, true, "bar", 4, 3, \"foo\"\]]=]
compare "-d\texamples/compare.rego\t-q\tdata.compare.output" [=[\[true, false, false, true\]]=]
keywords "-d\texamples/keywords.rego\texamples/data2.json\t-i\texamples/input0.json\t-q\tdata.keywords.output" [=[\[4, "found", \["0", "a"\], {"a": 3, "b": 5, "c": true, "sum": 8}, true\]]=]
imports "-d\texamples/scalars.rego\texamples/import0.rego\texamples/import1.rego\texamples/import2.rego\t-q\tdata.scalars.output" [=[\[305, true, 469\]]=]
with "-d\texamples/bodies.rego\texamples/with.rego\t-q\tdata.bodies.output" [=[\[8, 2, \[\[100, 300\], {"bar": 300, "foo": 200}\], 3, 0\]]=]
every "-d\texamples/every.rego\texamples/data2.json\t-q\tdata.every.output" "true"
compr "-d\texamples/compr.rego\texamples/data2.json\t-q\tdata.compr.output" "true"
setops "-d\texamples/setops.rego\t-q\tdata.setops.output" "true"
)

add_test(NAME rego_trieste COMMAND rego_trieste test -f WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_trieste>)
add_test(NAME query_math COMMAND rego_interpreter "-q" "x=5;x + (2 - 4 * 0.25) * -3 + 7.4")
set_tests_properties(query_math PROPERTIES PASS_REGULAR_EXPRESSION "x = 5[\r\n\t ]*9.4")

list(LENGTH tests_list tests_length)
while(${tests_length} GREATER 0)
  list(POP_FRONT tests_list test_name test_args test_regex)
  string(REPLACE "\t" ";" test_args_list ${test_args})
  add_test(NAME ${test_name}
           COMMAND rego_interpreter ${test_args_list} -a debug
           WORKING_DIRECTORY $<TARGET_FILE_DIR:rego_interpreter>)
  set_tests_properties(${test_name} PROPERTIES
           PASS_REGULAR_EXPRESSION "^${test_regex}")
  list(LENGTH tests_list tests_length)
endwhile()

add_custom_command(TARGET rego_interpreter POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples $<TARGET_FILE_DIR:rego_interpreter>/examples)
