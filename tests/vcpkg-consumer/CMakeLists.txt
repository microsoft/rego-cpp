cmake_minimum_required(VERSION 3.15)
project(regocpp-vcpkg-test CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

find_package(regocpp CONFIG REQUIRED)

add_executable(test-find-package test_find_package.cpp)
target_link_libraries(test-find-package PRIVATE regocpp::rego)
add_test(NAME find-package COMMAND test-find-package)
