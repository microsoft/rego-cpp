# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

set( REGO_CPP_VERSION 0.1.1 )
message("Configure REGO_CPP_VERSION at ${REGO_CPP_VERSION}")

project(regocpp VERSION ${REGO_CPP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/microsoft/trieste
  GIT_TAG        ea389c5834307840e5d4a128681789fe89cf9a3d
)

FetchContent_GetProperties(trieste)
if(NOT trieste_POPULATED)
  set(TRIESTE_BUILD_SAMPLES OFF)
  FetchContent_Populate(trieste)
  add_subdirectory(${trieste_SOURCE_DIR} ${trieste_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


find_program(CLANG_FORMAT NAMES clang-format-10 clang-format-14 )

string(COMPARE EQUAL ${CLANG_FORMAT} "CLANG_FORMAT-NOTFOUND" CLANG_FORMAT_NOT_FOUND)
if(CLANG_FORMAT_NOT_FOUND)
  message("regocpp_format target not defined: no clang-format tool found")
else()
  file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
        src/*.cc
        src/*.h
  )

  add_custom_target(regocpp_format
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND ${CLANG_FORMAT}
                    -i
                    ${ALL_SOURCE_FILES})
endif()

enable_testing()

add_subdirectory(src)

install(TARGETS rego_interpreter rego_trieste RUNTIME)
install(DIRECTORY src/examples DESTINATION .)
