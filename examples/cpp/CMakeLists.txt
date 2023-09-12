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

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

FetchContent_MakeAvailable(regocpp)

add_executable(custom_builtin custom_builtin.cc)
target_link_libraries(custom_builtin PRIVATE regocpp::rego)

install(TARGETS custom_builtin DESTINATION bin)

enable_testing()
add_test(
  NAME custom_builtin
  COMMAND bin/custom_builtin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist
)
set_tests_properties(custom_builtin PROPERTIES PASS_REGULAR_EXPRESSION "5")