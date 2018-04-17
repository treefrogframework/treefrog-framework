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
  $ENV{TreeFrog_HOME}/bin
)

set(TreeFrog_BIN_SEARCH_PATHS
  /usr/bin
  /usr/local/bin
  $ENV{TreeFrog_HOME}
  $ENV{TreeFrog_HOME}/bin
)

find_path(TreeFrog_INCLUDE_DIR NAMES TGlobal PATHS ${TreeFrog_INCLUDE_SEARCH_PATHS})
if(WIN32)
  find_library(TreeFrog_LIB NAMES treefrog1 PATHS ${TreeFrog_LIB_SEARCH_PATHS})
else()
  find_library(TreeFrog_LIB NAMES treefrog PATHS ${TreeFrog_LIB_SEARCH_PATHS})
endif()
find_program(TreeFrog_TMAKE_CMD tmake PATHS ${TreeFrog_BIN_SEARCH_PATHS})

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

# Check tmake command
if(NOT TreeFrog_TMAKE_CMD)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find tmake command. Turning TreeFrog_FOUND off")
endif()

if(TreeFrog_FOUND)
  message(STATUS "Found TreeFrog libraries: ${TreeFrog_LIB}")
  message(STATUS "Found TreeFrog include: ${TreeFrog_INCLUDE_DIR}")
  message(STATUS "Found TreeFrog tmake command: ${TreeFrog_TMAKE_CMD}")
else(TreeFrog_FOUND)
  message(FATAL_ERROR "Could not find TreeFrog")
endif(TreeFrog_FOUND)

mark_as_advanced(
  TreeFrog_INCLUDE_DIR
  TreeFrog_LIB
  TreeFrog_TMAKE_CMD
  TreeFrog 
)
