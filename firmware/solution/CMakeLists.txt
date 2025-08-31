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

# Executable
add_executable(answer main.cpp)

# Link libraries
target_link_libraries(answer dbcppp)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()


