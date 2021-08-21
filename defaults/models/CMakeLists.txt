add_definitions(-DTF_DLL)

find_package(Qt6 COMPONENTS Core Network Xml Sql Concurrent REQUIRED)
if (NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 was not found.")
endif()

file(GLOB model_headers ${PROJECT_SOURCE_DIR}/models/*.h)
file(GLOB model_srcs    ${PROJECT_SOURCE_DIR}/models/*.cpp)
file(GLOB model_objects_headers      ${PROJECT_SOURCE_DIR}/models/objects/*.h)
file(GLOB model_objects_srcs         ${PROJECT_SOURCE_DIR}/models/objects/*.cpp)
file(GLOB model_sqlobjects_headers   ${PROJECT_SOURCE_DIR}/models/sqlobjects/*.h)
file(GLOB model_mongoobjects_headers ${PROJECT_SOURCE_DIR}/models/mongoobjects/*.h)

add_library(model SHARED
  ${model_headers}
  ${model_srcs}
  ${model_objects_headers}
  ${model_objects_srcs}
  ${model_sqlobjects_headers}
  ${model_mongoobjects_headers}
)
target_include_directories(model PUBLIC
  ${Qt6Core_INCLUDE_DIRS}
  ${Qt6Network_INCLUDE_DIRS}
  ${Qt6Xml_INCLUDE_DIRS}
  ${Qt6Sql_INCLUDE_DIRS}
  ${Qt6Concurrent_INCLUDE_DIRS}
  ${TreeFrog_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/models
  ${PROJECT_SOURCE_DIR}/models/objects
  ${PROJECT_SOURCE_DIR}/models/sqlobjects
  ${PROJECT_SOURCE_DIR}/models/mongoobjects
)
target_link_libraries(model PRIVATE
  Qt6::Core
  Qt6::Network
  Qt6::Xml
  Qt6::Sql
  Qt6::Concurrent
  ${TreeFrog_LIB}
  helper
)
set_target_properties(model PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  SOVERSION 2.0
)
add_dependencies(model
  helper
)
