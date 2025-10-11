configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set( SOURCES
bigint.cc
interpreter.cc
bundle.cc
virtual_machine.cc
rego.cc
parse.cc
resolver.cc
builtins.cc
internal.cc
rego_c.cc
encoding.cc
json.cc
yaml.cc
file_to_rego.cc
rego_to_bundle.cc
bundle_binary.cc
bundle_json.cc
opblock.cc
dependency_graph.cc
internal.cc
builtins/aggregates.cc
builtins/arrays.cc
builtins/base64/base64.cpp
builtins/bits.cc
builtins/comparison.cc
builtins/conversions.cc
builtins/encoding.cc
builtins/graph.cc
builtins/internal.cc
builtins/numbers.cc
builtins/objects.cc
builtins/regex.cc
builtins/sets.cc
builtins/semver.cc
builtins/strings.cc
builtins/time.cc
builtins/types.cc
builtins/units.cc
builtins/uuid.cc
)

add_library(rego STATIC ${SOURCES})

add_library(regocpp::rego ALIAS rego)

target_link_libraries(rego
  PUBLIC
    trieste::trieste
    trieste::json
    trieste::yaml
)

if(MSVC)
  target_compile_options(rego PUBLIC "/Zc:__cplusplus")
  target_compile_definitions(rego PUBLIC "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" "DATE_BUILD_LIB")
endif()

if(Threads_FOUND)
  target_link_libraries(rego PUBLIC Threads::Threads)
endif()

if (REGOCPP_SANITIZE)
  target_compile_options(rego PUBLIC -g -fsanitize=${REGOCPP_SANITIZE} -fno-omit-frame-pointer)
  target_link_libraries(rego PUBLIC -fsanitize=${REGOCPP_SANITIZE})
endif()

target_compile_features(rego PUBLIC cxx_std_20)

if(REGOCPP_ACTION_METRICS)
  target_compile_definitions(rego PUBLIC REGOCPP_ACTION_METRICS)
endif()

target_include_directories( rego
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/rego>
)

if ( REGOCPP_BUILD_SHARED )
  add_library(rego_shared SHARED ${SOURCES})
  set_property(TARGET rego PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET yaml PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET json PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET snmalloc-new-override PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET re2 PROPERTY POSITION_INDEPENDENT_CODE ON)

  target_link_libraries(rego_shared
    PUBLIC
      trieste::trieste
      trieste::json
      trieste::yaml
  )

  if(MSVC)
    target_compile_options(rego_shared PUBLIC "/Zc:__cplusplus")
    target_compile_definitions(rego_shared PUBLIC "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" "REGO_SHARED" "DATE_BUILD_LIB")
  endif()

  if(Threads_FOUND)
    target_link_libraries(rego_shared PUBLIC Threads::Threads)
  endif()

  target_compile_features(rego_shared PUBLIC cxx_std_20)

  target_include_directories( rego_shared
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/rego>
  )
endif()
