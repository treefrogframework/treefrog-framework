add_definitions(-DTF_DLL)

find_package(Qt5 COMPONENTS Core Network Xml REQUIRED)

if(NOT Qt5_FOUND)
  message(FATAL_ERROR "Qt5 was not found. Consider setting QT5_CMAKE_PATH to the Qt5Config.cmake directory.")
endif()

execute_process(COMMAND tmake -f ${PROJECT_SOURCE_DIR}/config/application.ini -v ${PROJECT_SOURCE_DIR}/views -d ${CMAKE_CURRENT_BINARY_DIR}/gen -P)
add_custom_target(gen_view tmake -f ${PROJECT_SOURCE_DIR}/config/application.ini -v ${PROJECT_SOURCE_DIR}/views -d ${CMAKE_CURRENT_BINARY_DIR}/gen -P)

file(GLOB view_srcs ${CMAKE_CURRENT_BINARY_DIR}/gen/*.cpp)

unset(view_mocs)
foreach(_view_src ${view_srcs})
  string(REPLACE ".cpp" ".moc" _view_moc ${_view_src})
  qt5_generate_moc(${_view_src} ${_view_moc})
  list(APPEND view_mocs ${_view_moc})
endforeach()

add_library(view SHARED
  ${view_srcs}
  ${view_mocs}
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
  SOVERSION 1.0
)
add_dependencies(view
  helper
  model
  gen_view
)
