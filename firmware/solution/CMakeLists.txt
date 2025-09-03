cmake_minimum_required(VERSION 3.22)

project(solution)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable unnecessary dbcppp components
set(build_tools OFF CACHE BOOL "Build dbcppp utility application" FORCE)
set(build_tests OFF CACHE BOOL "Build tests" FORCE)
set(build_examples OFF CACHE BOOL "Build examples" FORCE)
set(build_kcd OFF CACHE BOOL "Enable support for KCD parsing" FORCE)

# Add dbcppp subdirectory
add_subdirectory(dbcppp)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/dbcppp/include)

# Main executable
add_executable(answer main.cpp)
target_link_libraries(answer dbcppp)

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    add_executable(tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/testmain.cpp
    )

    target_include_directories(tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_compile_features(tests PRIVATE cxx_std_17)

    add_test(NAME unit_tests COMMAND tests)
endif()

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()