# Cutom target - cmake
add_custom_target(cmake
   COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/CacheClean.cmake
   COMMAND echo "Command: ${CMAKE_COMMAND} -S ${PROJECT_SOURCE_DIR} -G \"${CMAKE_GENERATOR}\" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
   COMMAND ${CMAKE_COMMAND} -S ${PROJECT_SOURCE_DIR} -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
)

message(STATUS "Added a custom target for build: 'cmake'")
