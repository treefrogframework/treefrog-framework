# Clean cache
file(GLOB cmake_generated
  ${CMAKE_BINARY_DIR}/CMakeCache.txt
  ${CMAKE_BINARY_DIR}/cmake_install.cmake
  ${CMAKE_BINARY_DIR}/*/Makefile
  ${CMAKE_BINARY_DIR}/*/cmake_install.cmake
  ${CMAKE_BINARY_DIR}/views/*.cpp
)

foreach(file ${cmake_generated})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)
