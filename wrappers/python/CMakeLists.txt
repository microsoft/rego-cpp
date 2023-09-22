# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

project( regopy VERSION 0.1.0 LANGUAGES C CXX )

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
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11/
  GIT_TAG        v2.11.1
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 
set(SNMALLOC_ENABLE_DYNAMIC_LOADING ON)
set(REGOCPP_USE_CXX17 ON)

FetchContent_MakeAvailable(regocpp)
FetchContent_MakeAvailable(pybind11)

set(PYBIND11_CPP_STANDARD -std=c++17)
pybind11_add_module(_regopy src/_regopy.cc)
target_link_libraries(_regopy PRIVATE regocpp::rego)
set_property(TARGET rego PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET snmallocshim-static PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET re2 PROPERTY POSITION_INDEPENDENT_CODE ON)