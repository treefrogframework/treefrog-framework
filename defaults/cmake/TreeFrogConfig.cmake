# Search paths
file(TO_CMAKE_PATH "$ENV{TFDIR}" TFDIR)

set(TreeFrog_INCLUDE_SEARCH_PATHS
  /usr/include/treefrog
  /usr/local/include/treefrog
)

if (TFDIR)
  find_path(TreeFrog_INCLUDE_DIR TGlobal PATHS
    ${TFDIR}/include/treefrog
    ${TFDIR}/include
    NO_DEFAULT_PATH
  )

  set(TreeFrog_LIBNAME treefrog)
  if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(TreeFrog_LIBNAME treefrogd2)
      set(CMAKE_CXX_FLAGS "/EHsc /MDd")
    else ()
      set(TreeFrog_LIBNAME treefrog2)
      set(CMAKE_CXX_FLAGS "/EHsc /MD")
    endif()
  endif(MSVC)

  find_library(TreeFrog_LIB NAMES ${TreeFrog_LIBNAME} PATHS
    ${TFDIR}/lib
    ${TFDIR}/bin
    NO_DEFAULT_PATH
  )

  find_program(TreeFrog_TMAKE_CMD tmake PATHS ${TFDIR}/bin NO_DEFAULT_PATH)
else()
  find_path(TreeFrog_INCLUDE_DIR TGlobal PATHS ${TreeFrog_INCLUDE_SEARCH_PATHS})
  find_library(TreeFrog_LIB treefrog)
  find_program(TreeFrog_TMAKE_CMD tmake)
endif()

set(TreeFrog_FOUND ON)

# Check include files
if (NOT TreeFrog_INCLUDE_DIR)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find TreeFrog include file.")
endif()

# Check libraries
if (NOT TreeFrog_LIB)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find TreeFrog library.")
endif()

# Check tmake command
if (NOT TreeFrog_TMAKE_CMD)
  set(TreeFrog_FOUND OFF)
  message(STATUS "Could not find tmake command.")
endif()

if (TreeFrog_FOUND)
  message(STATUS "Found TreeFrog include: ${TreeFrog_INCLUDE_DIR}")
  message(STATUS "Found TreeFrog library: ${TreeFrog_LIB}")
  message(STATUS "Found TreeFrog tmake command: ${TreeFrog_TMAKE_CMD}")
else(TreeFrog_FOUND)
  message(FATAL_ERROR "Could not find TreeFrog. Set the install prefix to TFDIR environment variable.")
endif(TreeFrog_FOUND)

mark_as_advanced(
  TreeFrog_INCLUDE_DIR
  TreeFrog_LIB
  TreeFrog_TMAKE_CMD
)
