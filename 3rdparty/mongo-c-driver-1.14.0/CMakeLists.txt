cmake_minimum_required (VERSION 3.1)

# Used in MaintainerFlags.cmake to silence errors while testing configs.
set (MESSAGES_ENABLED 1)

function (message)
   list (GET ARGV 0 MessageType)
   if (MESSAGES_ENABLED)
      list (REMOVE_AT ARGV 0)
      _message (${MessageType} "${ARGV}")
   endif ()
endfunction ()

set (ENABLE_SSL AUTO CACHE STRING
   "Enable TLS connections and SCRAM-SHA-1 authentication. Options are
   \"DARWIN\" to use Apple's Secure Transport, \"WINDOWS\" to use Windows
   Secure Channel, \"OPENSSL\", \"LIBRESSL\", \"AUTO\",\ or \"OFF\". These options are
   case-sensitive. The default is \"AUTO\". Note\ that SCRAM-SHA-1 is
   required for authenticating to MongoDB 3.0 and later.")

set_property(CACHE ENABLE_SSL PROPERTY STRINGS "DARWIN;WINDOWS;OPENSSL;LIBRESSL;AUTO;OFF")

set (ENABLE_SASL AUTO CACHE STRING
   "Enable SASL authentication (Kerberos). Options are \"CYRUS\" to use Cyrus
   SASL, \"SSPI\" to use Windows Native SSPI, \"AUTO\",\ or \"OFF\". These options are case-sensitive.")

set_property(CACHE ENABLE_SASL PROPERTY STRINGS "CYRUS|SSPI|AUTO|OFF")

set (ENABLE_STATIC AUTO CACHE STRING "Build static libmongoc. Set to ON/AUTO/OFF, default AUTO.")
option (ENABLE_TESTS "Build MongoDB C Driver tests." ON)
option (ENABLE_EXAMPLES "Build MongoDB C Driver examples." ON)
set (ENABLE_SRV AUTO CACHE STRING "Support mongodb+srv URIs. Set to ON/AUTO/OFF, default AUTO.")
option (ENABLE_MAINTAINER_FLAGS "Use strict compiler checks" OFF)
option (ENABLE_AUTOMATIC_INIT_AND_CLEANUP "Enable automatic init and cleanup (GCC only)" ON)
option (ENABLE_CRYPTO_SYSTEM_PROFILE "Use system crypto profile (OpenSSL only)" OFF)
option (ENABLE_TRACING "Turn on verbose debug output" OFF)
option (ENABLE_COVERAGE "Turn on compile options for lcov" OFF)
set (ENABLE_SHM_COUNTERS AUTO CACHE STRING "Enable memory performance counters that use shared memory on Linux. Set to ON/AUTO/OFF, default AUTO.")
set (ENABLE_MONGOC ON CACHE STRING "Whether to build libmongoc. Set to ON/OFF, default ON.")
set (ENABLE_BSON AUTO CACHE STRING "Whether to build libbson. Set to ON/AUTO/SYSTEM, default AUTO.")
set (ENABLE_SNAPPY AUTO CACHE STRING "Enable snappy support. Set to ON/AUTO/OFF, default AUTO.")
set (ENABLE_ZLIB AUTO CACHE STRING "Enable zlib support")
option (ENABLE_MAN_PAGES "Build MongoDB C Driver manual pages." OFF)
option (ENABLE_HTML_DOCS "Build MongoDB C Driver HTML documentation." OFF)
option (ENABLE_EXTRA_ALIGNMENT
   "Turn on extra alignment of libbson types. Set to ON/OFF, default ON.\
 Required for the 1.0 ABI but better disabled."
   ON
)
option (ENABLE_RDTSCP
   "Fast performance counters on Intel using the RDTSCP instruction"
   OFF
)
option (ENABLE_APPLE_FRAMEWORK "Build libraries as frameworks on darwin platforms" OFF) 
set (ENABLE_ICU AUTO CACHE STRING "Enable ICU support, necessary to use non-ASCII usernames or passwords, default AUTO.")
option (ENABLE_UNINSTALL "Enable creation of uninstall script and associate uninstall build target." ON)

project (mongo-c-driver C)

if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "RelWithDebInfo")
   message (
      STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}"
   )
endif ()

set (CMAKE_MODULE_PATH
   ${CMAKE_MODULE_PATH}
   ${PROJECT_SOURCE_DIR}/build/cmake
   ${PROJECT_SOURCE_DIR}/build/cmake/make_dist
)

include (InstallRequiredSystemLibraries)
include (GNUInstallDirs)

set (BUILD_VERSION "0.0.0" CACHE STRING "Library version (for both libbson and libmongoc)")

include (ParseVersion)

# Set MONGOC_MAJOR_VERSION, MONGOC_MINOR_VERSION, etc.
if (BUILD_VERSION STREQUAL "0.0.0")
   if (EXISTS ${PROJECT_SOURCE_DIR}/VERSION_CURRENT)
      file (STRINGS ${PROJECT_SOURCE_DIR}/VERSION_CURRENT BUILD_VERSION)
      message ("file VERSION_CURRENT contained BUILD_VERSION ${BUILD_VERSION}")
   else ()
      find_package (PythonInterp)
      if (PYTHONINTERP_FOUND)
         execute_process (
            COMMAND ${PYTHON_EXECUTABLE} build/calc_release_version.py
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE CALC_RELEASE_VERSION
            RESULT_VARIABLE CALC_RELEASE_VERSION_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
         )
         if (NOT CALC_RELEASE_VERSION_RESULT STREQUAL 0)
            # If python failed above, stderr would tell the user about it
            message (WARNING
               "BUILD_VERSION not specified and could not be calculated\
 (script invocation failed); setting library version to 0.0.0"
            )
         else ()
            set (BUILD_VERSION ${CALC_RELEASE_VERSION})
            message ("calculated BUILD_VERSION ${BUILD_VERSION}")
         endif ()
      else ()
         message (WARNING
            "BUILD_VERSION not specified and could not be calculated\
 (Python was not found on the system); setting library version to 0.0.0"
         )
      endif ()
      message ("storing BUILD_VERSION ${BUILD_VERSION} in file VERSION_CURRENT for later use")
      file (WRITE ${PROJECT_SOURCE_DIR}/VERSION_CURRENT ${BUILD_VERSION})
   endif ()
else ()
   message ("storing BUILD_VERSION ${BUILD_VERSION} in file VERSION_CURRENT for later use")
   file (WRITE ${PROJECT_SOURCE_DIR}/VERSION_CURRENT ${BUILD_VERSION})
endif ()

include (LoadVersion)
LoadVersion (${PROJECT_SOURCE_DIR}/VERSION_CURRENT MONGOC)

include (MaintainerFlags)

if ( (ENABLE_BUILD_DEPENDECIES STREQUAL OFF) AND (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR) )
   set (ENABLE_BUILD_DEPENDECIES ON)
endif ()

if (ENABLE_EXTRA_ALIGNMENT STREQUAL ON)
   set (BSON_EXTRA_ALIGN 1)
else ()
   set (BSON_EXTRA_ALIGN 0)
endif ()

if (ENABLE_RDTSCP)
   set (MONGOC_ENABLE_RDTSCP 1)
else ()
   set (MONGOC_ENABLE_RDTSCP 0)
endif ()

if (NOT ENABLE_MONGOC MATCHES "ON|OFF")
   message (FATAL_ERROR "ENABLE_MONGOC option must be ON or OFF")
endif ()

if (NOT ENABLE_BSON MATCHES "ON|AUTO|SYSTEM")
   message (FATAL_ERROR "ENABLE_BSON option must be ON, AUTO, or SYSTEM")
endif ()

if (ENABLE_BSON STREQUAL SYSTEM)
   # The input variable BSON_ROOT_DIR is respected for backwards compatibility,
   # but you should use the standard CMAKE_PREFIX_PATH instead.
   message (STATUS "Searching for libbson CMake packages")
   find_package (libbson-1.0
      "${MONGOC_MAJOR_VERSION}.${MONGOC_MINOR_VERSION}.${MONGOC_MICRO_VERSION}"
      HINTS
      ${BSON_ROOT_DIR})

   if (ENABLE_BSON STREQUAL SYSTEM AND NOT BSON_LIBRARIES)
      message (FATAL_ERROR "System libbson not found")
   endif ()

   if (BSON_LIBRARIES)
      message ("--   libbson found version \"${BSON_VERSION}\"")
      message ("--   libbson include path \"${BSON_INCLUDE_DIRS}\"")
      message ("--   libbson libraries \"${BSON_LIBRARIES}\"")
      message ("--   disabling test-libmongoc since using system libbson")
      SET (ENABLE_TESTS OFF)

      if (ENABLE_STATIC MATCHES "ON|AUTO")
         find_package (libbson-static-1.0
            "${MONGOC_MAJOR_VERSION}.${MONGOC_MINOR_VERSION}.${MONGOC_MICRO_VERSION}"
            HINTS
            ${BSON_ROOT_DIR})

         if (ENABLE_STATIC STREQUAL ON AND NOT BSON_STATIC_LIBRARY)
            message (FATAL_ERROR "Static libbson not found. Pass -DENABLE_STATIC=OFF")
         endif ()

         if (BSON_STATIC_LIBRARY)
            set (MONGOC_ENABLE_STATIC ON)
         endif ()

         message ("--   libbson-static found version \"${BSON_STATIC_VERSION}\"")
         message ("--   libbson-static include path \"${BSON_STATIC_INCLUDE_DIRS}\"")
         message ("--   libbson-static libraries \"${BSON_STATIC_LIBRARIES}\"")
      endif ()
   endif ()
endif ()

unset (dist_generated CACHE)
unset (dist_generated_depends CACHE)

set (BUILD_SOURCE_DIR ${CMAKE_BINARY_DIR})

include (MakeDistFiles)

# Ensure the default behavior: don't ignore RPATH settings.
set (CMAKE_SKIP_BUILD_RPATH OFF)

# Ensure the default behavior: don't use the final install destination as the
# temporary RPATH for executables (ensure we can run tests and programs from
# the build directory).
set (CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

# Include any custom library paths in the final RPATH.
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

# Install libs with names like @rpath/libmongoc-1.0.0.dylib, not bare names.
set (CMAKE_MACOSX_RPATH ON)

# https://cmake.org/cmake/help/v3.11/policy/CMP0042.html
# Enable a CMake 3.0+ policy that sets CMAKE_MACOSX_RPATH by default, and
# silence a CMake 3.11 warning that the old behavior is deprecated.
cmake_policy (SET CMP0042 NEW)

if (NOT BSON_LIBRARIES)
   message ("  -- Using bundled libbson")
   if (ENABLE_STATIC MATCHES "ON|AUTO")
      set (MONGOC_ENABLE_STATIC ON)
   endif ()
   add_subdirectory (src/libbson)
   # Defined in src/libbson/CMakeLists.txt
   set (BSON_STATIC_LIBRARIES bson_static)
   set (BSON_LIBRARIES bson_shared)
   set (BSON_STATIC_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/libbson/src" "${PROJECT_BINARY_DIR}/src/libbson/src")
   set (BSON_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/libbson/src" "${PROJECT_BINARY_DIR}/src/libbson/src")
   set (BSON_STATIC_DEFINITIONS "BSON_STATIC")
endif ()

if (MSVC)
   add_definitions (-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_definitions (-D_GNU_SOURCE)
add_definitions (-D_BSD_SOURCE)
add_definitions (-D_DEFAULT_SOURCE)

if (ENABLE_MONGOC)

   if (ENABLE_TESTS AND NOT MONGOC_ENABLE_STATIC)
      message (FATAL_ERROR "ENABLE_TESTS requires ENABLE_STATIC")
   endif ()

   if (NOT ENABLE_SSL MATCHES "DARWIN|WINDOWS|OPENSSL|LIBRESSL|AUTO|OFF")
      message (FATAL_ERROR
               "ENABLE_SSL option must be DARWIN, WINDOWS, OPENSSL, LIBRESSL, AUTO, or OFF")
   endif ()

   set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

   set (ZLIB_SOURCES
      ${SOURCE_DIR}/src/zlib-1.2.11/adler32.c
      ${SOURCE_DIR}/src/zlib-1.2.11/crc32.c
      ${SOURCE_DIR}/src/zlib-1.2.11/deflate.c
      ${SOURCE_DIR}/src/zlib-1.2.11/infback.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inffast.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inflate.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inftrees.c
      ${SOURCE_DIR}/src/zlib-1.2.11/trees.c
      ${SOURCE_DIR}/src/zlib-1.2.11/zutil.c
      ${SOURCE_DIR}/src/zlib-1.2.11/compress.c
      ${SOURCE_DIR}/src/zlib-1.2.11/uncompr.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzclose.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzlib.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzread.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzwrite.c
   )

   set (MONGOC_ENABLE_ICU 0)

   set (CPACK_RESOURCE_FILE_LICENSE "${SOURCE_DIR}/COPYING")

   include (CPack)

   # Ensure the default behavior: don't ignore RPATH settings.
   set (CMAKE_SKIP_BUILD_RPATH OFF)

   if (APPLE)
      # Until CDRIVER-520.
      add_definitions (-Wno-deprecated-declarations)
   endif ()

   add_subdirectory (src/libmongoc)

   if (ENABLE_MAN_PAGES STREQUAL ON OR ENABLE_HTML_DOCS STREQUAL ON)
      find_package (Sphinx REQUIRED)
      add_custom_target (doc
         ALL
         DEPENDS
            $<$<STREQUAL:"${ENABLE_BSON}","ON">:bson-doc>
            $<$<STREQUAL:"${ENABLE_MONGOC}","ON">:mongoc-doc>
      )
   endif ()

   # Implement 'dist' and 'distcheck' targets
   #
   # CMake does not implement anything like 'dist' and 'distcheck' from autotools.
   # This implementation is based on the one in GnuCash.

   add_subdirectory (build)
   # sub-directory 'doc' was already included above
   add_subdirectory (orchestration_configs)
   add_subdirectory (src)
   # 'src/libbson' was already included, so 'src' will not include it directly
   # 'src/libmongoc' was already included, so 'src' will not include it directly

   set (PACKAGE_PREFIX "mongo-c-driver-${MONGOC_DIST_VERSION}")
   set (DIST_FILE "${PACKAGE_PREFIX}.tar.gz")

   set (top_DIST_local
      CMakeLists.txt
      CONTRIBUTING.md
      COPYING
      NEWS
      README.rst
      THIRD_PARTY_NOTICES
      VERSION_CURRENT
      # This sub-directory is added later, so manually include here
      generate_uninstall/CMakeLists.txt
   )

   set_local_dist (top_DIST ${top_DIST_local})

   set (ALL_DIST
      ${top_DIST}
      ${build_DIST}
      ${orchestration_configs_DIST}
      ${src_DIST}
      ${src_libbson_DIST}
      ${src_libmongoc_DIST}
   )

   # Write a dist manifest
   string (REPLACE ";" "\n" ALL_DIST_LINES "${ALL_DIST}")
   file (WRITE ${CMAKE_BINARY_DIR}/dist_manifest.txt ${ALL_DIST_LINES})

   # This is the command that produces the distribution tarball
   add_custom_command (OUTPUT ${DIST_FILE}
      COMMAND ${CMAKE_COMMAND}
         -D CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/build/cmake/make_dist
         -D PACKAGE_PREFIX=${PACKAGE_PREFIX}
         -D MONGOC_SOURCE_DIR=${CMAKE_SOURCE_DIR}
         -D BUILD_SOURCE_DIR=${BUILD_SOURCE_DIR}
         -D SHELL=${SHELL}
         "-Ddist_generated=\"${dist_generated}\""
         -P ${PROJECT_SOURCE_DIR}/build/cmake/make_dist/MakeDist.cmake

      DEPENDS
         ${ALL_DIST} ${dist_generated_depends}
   )

   if (ENABLE_BSON MATCHES "ON|AUTO" AND ENABLE_MAN_PAGES STREQUAL ON AND ENABLE_HTML_DOCS STREQUAL ON)
      # Since our 'dist' implementation does not add top-level targets for every
      # file to be included, we declare a dependency on the 'mongo-doc' target so
      # that documentation is built before the distribution tarball is generated.
      add_custom_target (dist DEPENDS doc ${DIST_FILE})

      add_custom_target (distcheck DEPENDS dist
         COMMAND ${CMAKE_COMMAND}
            -D CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/build/cmake/make_dist
            -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
            -D PACKAGE_PREFIX=${PACKAGE_PREFIX}
            -D CMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -D CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -P ${PROJECT_SOURCE_DIR}/build/cmake/make_dist/MakeDistCheck.cmake
      )
   else ()
      string (CONCAT DISTERRMSG
         "The dist and distcheck targets are disabled. Set ENABLE_BSON=ON, "
         "ENABLE_MAN_PAGES=ON, ENABLE_HTML_DOCS=ON, and generate VERSION_CURRENT "
         "to enable."
      )

      add_custom_target (dist
         COMMAND ${CMAKE_COMMAND} -E echo "${DISTERRMSG}"
      )

      add_custom_target (distcheck
         COMMAND ${CMAKE_COMMAND} -E echo "${DISTERRMSG}"
      )
   endif ()

endif ()

install (FILES COPYING NEWS README.rst THIRD_PARTY_NOTICES
   DESTINATION ${CMAKE_INSTALL_DATADIR}/mongo-c-driver
)

if (ENABLE_UNINSTALL)
   if (WIN32)
      if (ENABLE_MONGOC)
         set (UNINSTALL_PROG "uninstall.cmd")
      else ()
         set (UNINSTALL_PROG "uninstall-bson.cmd")
      endif ()
   else ()
      if (ENABLE_MONGOC)
         set (UNINSTALL_PROG "uninstall.sh")
      else ()
         set (UNINSTALL_PROG "uninstall-bson.sh")
      endif ()
   endif ()
   set (UNINSTALL_PROG_DIR "${CMAKE_INSTALL_DATADIR}/mongo-c-driver")

   # Create uninstall program and associated uninstall target
   #
   # This needs to be last (after all other add_subdirectory calls) to ensure that
   # the generated uninstall program is complete and correct
   add_subdirectory (generate_uninstall)
endif ()
