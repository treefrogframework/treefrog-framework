add_definitions(-DTF_DLL)

find_package(Qt6 COMPONENTS Core Network Xml Sql REQUIRED)

if (NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 was not found.")
endif()

file(GLOB controller_headers ${PROJECT_SOURCE_DIR}/controllers/*.h)
file(GLOB controller_srcs ${PROJECT_SOURCE_DIR}/controllers/*.cpp)

add_library(controller SHARED
  ${controller_headers}
  ${controller_srcs}
)
target_include_directories(controller PUBLIC
  ${Qt6Core_INCLUDE_DIRS}
  ${Qt6Network_INCLUDE_DIRS}
  ${Qt6Xml_INCLUDE_DIRS}
  ${Qt6Sql_INCLUDE_DIRS}
  ${TreeFrog_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/helpers
  ${PROJECT_SOURCE_DIR}/models
)
target_link_libraries(controller PRIVATE
  Qt6::Core
  Qt6::Network
  Qt6::Xml
  Qt6::Sql
  ${TreeFrog_LIB}
  helper
  model
)
set_target_properties(controller PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  SOVERSION 2.0
)
add_dependencies(controller
  helper
  model
)
