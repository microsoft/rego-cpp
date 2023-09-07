# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

project( regoc VERSION 0.1.0 LANGUAGES C CXX )

FetchContent_Declare(
  regocpp
  GIT_REPOSITORY https://github.com/matajoh/rego-cpp
  GIT_TAG        15c65c1d5212f30ae414f6eb4410813d8923a43e
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

FetchContent_MakeAvailable(regocpp)

add_executable(regoc main.c)
target_link_libraries(regoc PRIVATE regocpp::rego)

install(TARGETS regoc DESTINATION bin)