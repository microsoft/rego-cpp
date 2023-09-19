# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

project( regoc VERSION 0.1.0 LANGUAGES C CXX )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (DEFINED ENV{REGOCPP_C_WRAPPER_REPO})
  set(REGOCPP_C_WRAPPER_REPO $ENV{REGOCPP_C_WRAPPER_REPO})
else ()
  set(REGOCPP_C_WRAPPER_REPO "https://github.com/microsoft/rego-cpp/")
endif ()

if (DEFINED ENV{REGOCPP_C_WRAPPER_TAG})
  set(REGOCPP_C_WRAPPER_TAG $ENV{REGOCPP_C_WRAPPER_TAG})
else ()
  set(REGOCPP_C_WRAPPER_TAG "main")
endif()

FetchContent_Declare(
  regocpp
  GIT_REPOSITORY ${REGOCPP_C_WRAPPER_REPO}
  GIT_TAG        ${REGOCPP_C_WRAPPER_TAG}
)

FetchContent_Declare(
  cargs
  GIT_REPOSITORY https://github.com/likle/cargs
  GIT_TAG        v1.0.3
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

FetchContent_MakeAvailable(regocpp)
FetchContent_MakeAvailable(cargs)

add_executable(regoc main.c)
target_link_libraries(regoc PRIVATE regocpp::rego cargs)

add_executable(example example.c)
target_link_libraries(example PRIVATE regocpp::rego)

install(TARGETS regoc example DESTINATION bin)

enable_testing()
add_test(
  NAME regoc
  COMMAND bin/regoc -d examples/data0.json -d examples/data1.json -i examples/input0.json -d examples/objects.rego -q data.objects.sites[1] 
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist
)
set_tests_properties(regoc PROPERTIES PASS_REGULAR_EXPRESSION "{\"name\":\"smoke1\"}")