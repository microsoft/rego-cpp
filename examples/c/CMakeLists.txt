# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

project( regoc VERSION 0.1.0 LANGUAGES C CXX )

FetchContent_Declare(
  regocpp
  GIT_REPOSITORY https://github.com/matajoh/rego-cpp
  GIT_TAG        a33dad922de61eca609345ebcbcc1d3d4cd2743b
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

install(TARGETS regoc DESTINATION bin)