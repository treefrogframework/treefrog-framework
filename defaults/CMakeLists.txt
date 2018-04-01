cmake_minimum_required(VERSION 3.0)

project(appname CXX)
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -W -D_REENTRANT")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -W -D_REENTRANT -DQT_NO_DEBUG")

include(cmake/FindTreeFrog.cmake)
add_subdirectory(helpers)
add_subdirectory(models)
add_subdirectory(controllers)
add_subdirectory(views)
