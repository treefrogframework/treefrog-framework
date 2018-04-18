# Search paths
set(TFDIR $ENV{TFDIR})

set(TreeFrog_INCLUDE_SEARCH_PATHS
  /usr/include/treefrog
  /usr/local/include/treefrog
)

if(TFDIR)
  find_path(TreeFrog_INCLUDE_DIR TGlobal PATHS ${TFDIR}/include/treefrog NO_DEFAULT_PATH)
  find_library(TreeFrog_LIB treefrog PATHS ${TFDIR}/lib NO_DEFAULT_PATH)
  find_program(TreeFrog_TMAKE_CMD tmake PATHS ${TFDIR}/bin NO_DEFAULT_PATH)
else()
  find_path(TreeFrog_INCLUDE_DIR TGlobal PATHS ${TreeFrog_INCLUDE_SEARCH_PATHS})
  find_library(TreeFrog_LIB treefrog)
  find_program(TreeFrog_TMAKE_CMD tmake)
endif()

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
  message(STATUS "Found TreeFrog include: ${TreeFrog_INCLUDE_DIR}")
  message(STATUS "Found TreeFrog libraries: ${TreeFrog_LIB}")
  message(STATUS "Found TreeFrog tmake command: ${TreeFrog_TMAKE_CMD}")
else(TreeFrog_FOUND)
  message(FATAL_ERROR "Could not find TreeFrog. Set the path prefix to TFDIR environment variable.")
endif(TreeFrog_FOUND)

mark_as_advanced(
  TreeFrog_INCLUDE_DIR
  TreeFrog_LIB
  TreeFrog_TMAKE_CMD
)
