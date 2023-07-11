configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set( SOURCES
interpreter.cc
lang.cc
parse.cc
args.cc
unifier.cc
value.cc
value_map.cc
resolver.cc
variable.cc
wf.cc
passes/input_data.cc
passes/modules.cc
passes/lists.cc
passes/imports.cc
passes/keywords.cc
passes/if_else.cc
passes/structure.cc
passes/strings.cc
passes/symbols.cc
passes/locals.cc
passes/multiply_divide.cc
passes/add_subtract.cc
passes/comparison.cc
passes/assign.cc
passes/rulebody.cc
passes/refs.cc
passes/functions.cc
passes/merge_data.cc
passes/merge_modules.cc
passes/rules.cc
passes/query.cc
)

add_library(rego STATIC ${SOURCES})

add_library(regocpp::rego ALIAS rego)

if ( REGOCPP_LOGGING )
  target_compile_definitions(rego PUBLIC REGOCPP_LOGGING)
endif()

target_link_libraries(rego
  PUBLIC
    trieste::trieste
)

target_include_directories( rego
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/rego>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
