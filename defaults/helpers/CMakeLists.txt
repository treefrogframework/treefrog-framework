add_definitions(-DTF_DLL)

find_package(Qt6 COMPONENTS Core REQUIRED)

if (NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 was not found.")
endif()

file(GLOB helper_srcs    ${PROJECT_SOURCE_DIR}/helpers/*.cpp)
file(GLOB helper_headers ${PROJECT_SOURCE_DIR}/helpers/*.h)

add_library(helper SHARED
  ${helper_srcs}
)
target_include_directories(helper PUBLIC
  ${Qt6Core_INCLUDE_DIRS}
  ${TreeFrog_INCLUDE_DIR}
)
target_link_libraries(helper PRIVATE
  Qt6::Core
  ${TreeFrog_LIB}
)
set_target_properties(helper PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  SOVERSION 2.0
)
