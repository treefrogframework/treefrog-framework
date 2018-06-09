# Cutom target - cmake
add_custom_target(cmake
   COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/CacheClean.cmake
   COMMAND echo "Command: ${CMAKE_COMMAND} -G \"${CMAKE_GENERATOR}\" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${PROJECT_SOURCE_DIR}"
   COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${PROJECT_SOURCE_DIR}
)

message(STATUS "Added a custom target for build: 'cmake'")
