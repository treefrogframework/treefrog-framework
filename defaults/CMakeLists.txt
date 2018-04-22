cmake_minimum_required(VERSION 3.0)

project(appname CXX)
set(default_build_type "Release")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# cxx options
set(CMAKE_CXX_STANDARD 11)
if (MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG   "/DEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DQT_NO_DEBUG /DTF_NO_DEBUG")
else()
  set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -W -D_REENTRANT")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -W -D_REENTRANT -DQT_NO_DEBUG -DTF_NO_DEBUG")
endif()

# Auto generate moc files
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()
set(CMAKE_AUTOMOC ON)

include(cmake/FindTreeFrog.cmake)
add_subdirectory(helpers)
add_subdirectory(models)
add_subdirectory(views)
add_subdirectory(controllers)
