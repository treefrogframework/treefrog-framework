add_definitions(-DTF_DLL)

find_package(Qt5 COMPONENTS Core REQUIRED)

if (NOT Qt5_FOUND)
  message(FATAL_ERROR "Qt5 was not found. Consider setting QT5_CMAKE_PATH to the Qt5Config.cmake directory.")
endif()

file(GLOB helper_srcs    ${PROJECT_SOURCE_DIR}/helpers/*.cpp)
file(GLOB helper_headers ${PROJECT_SOURCE_DIR}/helpers/*.h)

add_library(helper SHARED
  ${helper_srcs}
)
target_include_directories(helper PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${TreeFrog_INCLUDE_DIR}
)
target_link_libraries(helper
  Qt5::Core
  ${TreeFrog_LIB}
)
set_target_properties(helper PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  SOVERSION 1.0
)
