# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

file( STRINGS "VERSION" REGOCPP_VERSION_FILE )

string( REPLACE "." ";" REGOCPP_VERSION_LIST ${REGOCPP_VERSION_FILE} )

list( GET REGOCPP_VERSION_LIST 0 REGOCPP_VERSION_MAJOR )

list( GET REGOCPP_VERSION_LIST 1 REGOCPP_VERSION_MINOR )

list( GET REGOCPP_VERSION_LIST 2 REGOCPP_VERSION_REVISION )

set ( REGOCPP_OPA_VERSION 0.68.0 )

set( REGOCPP_VERSION ${REGOCPP_VERSION_MAJOR}.${REGOCPP_VERSION_MINOR}.${REGOCPP_VERSION_REVISION} )

message("Configure REGOCPP_VERSION at ${REGOCPP_VERSION}")

project( regocpp VERSION ${REGOCPP_VERSION} LANGUAGES CXX)

# -------------------- Get version info --------------------------

find_package(Git REQUIRED)

execute_process(
  COMMAND git log -1 --format=%H
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE REGOCPP_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE REGOCPP_GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE REGOCPP_GIT_SHORT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

execute_process(
  COMMAND git log -1 --format=%aD
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE REGOCPP_BUILD_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

set(REGOCPP_BUILD_TOOLCHAIN "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")


# -------------------- Options --------------------------

option(REGOCPP_BUILD_TOOLS "Specifies whether to build the command line tools" OFF)
option(REGOCPP_BUILD_TESTS "Specifies whether to build the tests" OFF)
option(REGOCPP_BUILD_DOCS "Specifies whether to build the documentation" OFF)
option(REGOCPP_BUILD_SHARED "Whether to build the rego_shared library" OFF)
option(REGOCPP_OPA_TESTS "Specifies whether to load and run the OPA built-ins tests" OFF)
option(REGOCPP_COPY_EXAMPLES "Specifies whether to copy the examples to the install directory" OFF)
option(REGOCPP_ACTION_METRICS "Specifies whether to metricate Trieste Actions" OFF)
option(REGOCPP_CLEAN_INSTALL "Whether the install directory should be cleaned before install" OFF)
set(REGOCPP_SANITIZE "" CACHE STRING "Argument to pass to sanitize (disabled by default)")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------- Find packages --------------------------

find_package(Threads)

# Used to provide
# FetchContent_MakeAvailable_ExcludeFromAll
FetchContent_Declare(
    cmake_utils
    GIT_REPOSITORY https://github.com/mjp41/cmake_utils
    GIT_TAG 2bf98b5773ea7282197c823e205547d8c2e323c0
    GIT_SHALLOW FALSE
)

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/microsoft/trieste
  GIT_TAG        44e71e994b03ef766357167df32f148b03a97aa4
)

FetchContent_MakeAvailable(cmake_utils)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

set(TRIESTE_BUILD_SAMPLES OFF)
set(TRIESTE_BUILD_PARSERS ON)

FetchContent_MakeAvailable_ExcludeFromAll(trieste)

find_program(CLANG_FORMAT NAMES clang-format-10 clang-format-14 clang-format-18 )

string(COMPARE EQUAL ${CLANG_FORMAT} "CLANG_FORMAT-NOTFOUND" CLANG_FORMAT_NOT_FOUND)
if(CLANG_FORMAT_NOT_FOUND)
  message("regocpp_format target not defined: no clang-format tool found")
else()
  file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
        src/*.cc
        src/*.hh
        src/reader/*.cc
        src/unify/*.cc
        src/*.h
        include/rego/*.h
        include/rego/*.hh
        tools/*.cc
        tests/*.cc
        tests/*.h
        examples/c/*.c
        examples/cpp/*.cc
  )

  add_custom_target(regocpp_format
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND ${CLANG_FORMAT}
                    -i
                    ${ALL_SOURCE_FILES})
endif()

if(REGOCPP_OPA_TESTS)
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opa)
    execute_process(COMMAND ${GIT_EXECUTABLE} clone --depth=1 --branch v${REGOCPP_OPA_VERSION} https://github.com/open-policy-agent/opa/ 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    OUTPUT_QUIET)
  endif()
endif()

find_program(CCACHE_PATH ccache)
if(CCACHE_PATH)
  message("Using ccache")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PATH})
endif()

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# -------------------- Walk the subdirectories --------------------

if(REGOCPP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

add_subdirectory(src)

if(REGOCPP_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(REGOCPP_BUILD_DOCS)
  find_package( Doxygen REQUIRED )
  add_subdirectory( doc )
endif()

# -------------------- Install ------------------------------------

if(REGOCPP_CLEAN_INSTALL)
  message("${CMAKE_INSTALL_PREFIX} will be recursively cleaned before install")
  # Clear all existing files and folders from the install directory
  install(CODE [[
    file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/.)
    ]])
endif()

set(INSTALL_CONFIGDIR cmake)
set(INSTALL_LIBDIR lib)
set(INSTALL_INCLUDEDIR include)
set(REGOCPP_INSTALL_TARGETS rego trieste json yaml snmalloc re2)

if(REGOCPP_BUILD_SHARED)
  list(APPEND REGOCPP_INSTALL_TARGETS rego_shared)
endif()

if(TRIESTE_USE_SNMALLOC)
  list(APPEND REGOCPP_INSTALL_TARGETS snmalloc-new-override)
endif()

install(TARGETS ${REGOCPP_INSTALL_TARGETS}
  EXPORT ${PROJECT_NAME}_Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION
  ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

install(EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

if (REGOCPP_BUILD_TOOLS)
  install(TARGETS rego_interpreter rego_fuzzer RUNTIME)
endif()

if (REGOCPP_COPY_EXAMPLES)
  install(DIRECTORY tools/examples DESTINATION .)
endif()

if (REGOCPP_BUILD_TESTS)
  install(TARGETS rego_test rego_test_c_api RUNTIME)
  install(FILES tests/regocpp.yaml tests/bigint.yaml tests/bugs.yaml tests/regov1.yaml tests/bad.json DESTINATION tests)
  install(DIRECTORY tests/aci DESTINATION tests)
endif()

export(EXPORT ${PROJECT_NAME}_Targets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::)

export(PACKAGE ${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION ${INSTALL_INCLUDEDIR})
install(FILES 
  ${CMAKE_CURRENT_BINARY_DIR}/src/version.h  
  DESTINATION ${INSTALL_INCLUDEDIR}/rego
)
