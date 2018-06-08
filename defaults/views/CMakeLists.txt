add_definitions(-DTF_DLL)

find_package(Qt5 COMPONENTS Core Network Xml REQUIRED)

if (NOT Qt5_FOUND)
  message(FATAL_ERROR "Qt5 was not found. Consider setting QT5_CMAKE_PATH to the Qt5Config.cmake directory.")
endif()

execute_process(COMMAND
  ${TreeFrog_TMAKE_CMD} -f ${PROJECT_SOURCE_DIR}/config/application.ini -v ${PROJECT_SOURCE_DIR}/views -d ${CMAKE_CURRENT_BINARY_DIR} -P
)
add_custom_target(genview ALL
  ${TreeFrog_TMAKE_CMD} -f ${PROJECT_SOURCE_DIR}/config/application.ini -v ${PROJECT_SOURCE_DIR}/views -d ${CMAKE_CURRENT_BINARY_DIR} -P
)

file(GLOB view_srcs ${CMAKE_CURRENT_BINARY_DIR}/*.cpp)

add_library(view SHARED
  ${view_srcs}
)
target_include_directories(view PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
  ${Qt5Xml_INCLUDE_DIRS}
  ${TreeFrog_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/helpers
  ${PROJECT_SOURCE_DIR}/models
)
target_link_libraries(view
  Qt5::Core
  Qt5::Network
  Qt5::Xml
  ${TreeFrog_LIB}
  helper
  model
)
set_target_properties(view PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/lib
  SOVERSION 1.0
)
add_dependencies(view
  helper
  model
  genview
)
