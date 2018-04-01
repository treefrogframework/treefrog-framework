set(TreeFrog_INCLUDE_SEARCH_PATHS
  /usr/include/treefrog
  /usr/local/include/treefrog
  $ENV{TreeFrog_HOME}
  $ENV{TreeFrog_HOME}/include
  $ENV{TreeFrog_HOME}/include/treefrog
)

set(TreeFrog_LIB_SEARCH_PATHS
  /usr/lib
  /usr/local/lib
  $ENV{TreeFrog_HOME}
  $ENV{TreeFrog_HOME}/lib
)

find_path(TreeFrog_INCLUDE_DIR NAMES TGlobal PATHS ${TreeFrog_INCLUDE_SEARCH_PATHS})
find_library(TreeFrog_LIB NAMES treefrog PATHS ${TreeFrog_LIB_SEARCH_PATHS})

set(TreeFrog_FOUND ON)

# Check include files
if(NOT TreeFrog_INCLUDE_DIR)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find TreeFrog include. Turning TreeFrog_FOUND off")
endif()

# Check libraries
if(NOT TreeFrog_LIB)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find TreeFrog lib. Turning TreeFrog_FOUND off")
endif()

if(TreeFrog_FOUND)
  if(NOT TreeFrog_FIND_QUIETLY)
    message(STATUS "Found TreeFrog libraries: ${TreeFrog_LIB}")
    message(STATUS "Found TreeFrog include: ${TreeFrog_INCLUDE_DIR}")
  endif(NOT TreeFrog_FIND_QUIETLY)
else(TreeFrog_FOUND)
  if(TreeFrog_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find TreeFrog")
  endif(TreeFrog_FIND_REQUIRED)
endif(TreeFrog_FOUND)

mark_as_advanced(
  TreeFrog_INCLUDE_DIR
  TreeFrog_LIB
  TreeFrog 
)
