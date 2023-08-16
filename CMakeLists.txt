# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)

file( STRINGS "VERSION" REGOCPP_VERSION_FILE )

string( REPLACE "." ";" REGOCPP_VERSION_LIST ${REGOCPP_VERSION_FILE} )

list( GET REGOCPP_VERSION_LIST 0 REGOCPP_VERSION_MAJOR )

list( GET REGOCPP_VERSION_LIST 1 REGOCPP_VERSION_MINOR )

list( GET REGOCPP_VERSION_LIST 2 REGOCPP_VERSION_REVISION )

set( REGOCPP_VERSION ${REGOCPP_VERSION_MAJOR}.${REGOCPP_VERSION_MINOR}.${REGOCPP_VERSION_REVISION} )

message("Configure REGOCPP_VERSION at ${REGOCPP_VERSION}")

project( regocpp VERSION ${REGOCPP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------- Options --------------------------

option(REGOCPP_BUILD_TOOLS "Specifies whether to build the command line tools" OFF)
option(REGOCPP_BUILD_TESTS "Specifies whether to build the tests" OFF)
option(REGOCPP_OPA_TESTS "Specifies whether to load and run the OPA built-ins tests" OFF)

# -------------------- Find packages --------------------------

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/microsoft/trieste
  GIT_TAG        949c46601db56056282bacf0b41b12a101d09616
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) 

set(TRIESTE_BUILD_SAMPLES OFF)
FetchContent_MakeAvailable(trieste)

find_program(CLANG_FORMAT NAMES clang-format-10 clang-format-14 )

string(COMPARE EQUAL ${CLANG_FORMAT} "CLANG_FORMAT-NOTFOUND" CLANG_FORMAT_NOT_FOUND)
if(CLANG_FORMAT_NOT_FOUND)
  message("regocpp_format target not defined: no clang-format tool found")
else()
  file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
        src/*.cc
        src/passes/*.cc
        src/*.h
        include/rego/*.h
        tools/*.cc
        tests/*.cc
        tests/*.h
  )

  add_custom_target(regocpp_format
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMAND ${CLANG_FORMAT}
                    -i
                    ${ALL_SOURCE_FILES})
endif()

if(REGOCPP_OPA_TESTS)
  find_package(Git REQUIRED)
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opa)
    execute_process(COMMAND ${GIT_EXECUTABLE} clone --depth=1 --branch v0.55.0 https://github.com/open-policy-agent/opa/ 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    OUTPUT_QUIET)
  endif()
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

# -------------------- Install ------------------------------------

set(INSTALL_CONFIGDIR cmake)
set(INSTALL_LIBDIR lib)
set(INSTALL_INCLUDEDIR include)

install(TARGETS rego
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
  install(TARGETS rego_interpreter rego_trieste RUNTIME)
  install(DIRECTORY tools/examples DESTINATION .)
endif()

if (REGOCPP_BUILD_TESTS)
  install(TARGETS rego_test yaml_trieste RUNTIME)
  install(FILES tests/regocpp.yaml tests/bigint.yaml DESTINATION tests)
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
