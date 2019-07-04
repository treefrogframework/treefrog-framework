cmake_minimum_required(VERSION 3.1)

project(appname CXX)

# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# cxx options
if (MSVC)
  set(CMAKE_CXX_FLAGS         "/DUNICODE /D_UNICODE")
  set(CMAKE_CXX_FLAGS_DEBUG   "/DEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DQT_NO_DEBUG /DTF_NO_DEBUG")
else()
  set(CMAKE_CXX_FLAGS         "-Wall -W -D_REENTRANT")
  set(CMAKE_CXX_FLAGS_DEBUG   "-g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DQT_NO_DEBUG -DTF_NO_DEBUG")
endif()

# Auto generate moc files
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()
set(CMAKE_AUTOMOC ON)

find_package(TreeFrog REQUIRED)
add_subdirectory(helpers)
add_subdirectory(models)
add_subdirectory(views)
add_subdirectory(controllers)

message(STATUS "Set CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
message(STATUS "Set CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Cutom target - cmake
include(${PROJECT_SOURCE_DIR}/cmake/TargetCmake.cmake)
