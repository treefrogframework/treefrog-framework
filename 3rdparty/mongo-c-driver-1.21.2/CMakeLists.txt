cmake_minimum_required (VERSION 3.1)

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

set (ENABLE_STATIC AUTO CACHE STRING "Build and install static libbson/libmongoc. Set to ON/AUTO/OFF/BUILD_ONLY/DONT_INSTALL, default AUTO.")
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
set (ENABLE_ZSTD AUTO CACHE STRING "Enable zstd support. Set to ON/AUTO/OFF, default AUTO.")
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
option (ENABLE_UNINSTALL "Enable creation of uninstall script and associated uninstall build target." ON)
set (ENABLE_CLIENT_SIDE_ENCRYPTION AUTO CACHE STRING "Enable Client-Side Field Level Encryption support. Requires libmongocrypt. Set to ON/AUTO/OFF, default AUTO.")
set (ENABLE_MONGODB_AWS_AUTH AUTO CACHE STRING "Enable support for MONGODB-AWS authentication mechanism. Set to ON/AUTO/OFF, default AUTO. (also requires ENABLE_SSL not set to OFF)")
option (ENABLE_PIC
   "Enables building of position independent code for static library components."
   ON
)
option (ENABLE_DEBUG_ASSERTIONS "Turn on runtime debug assertions" OFF)

project (mongo-c-driver C)

# Optionally enable C++ to do some C++-specific tests
include (CheckLanguage)
check_language (CXX)
if (CMAKE_CXX_COMPILER)
   enable_language (CXX)
else ()
   message (STATUS "No CXX support")
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
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

include(MongoC-Warnings)

# Enable CCache, if possible
include (CCache)
# Link with LLD, if possible
include (LLDLinker)

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
            message (FATAL_ERROR
               "BUILD_VERSION not specified and could not be calculated\
 (script invocation failed); specify in CMake command, -DBUILD_VERSION=<version>"
            )
         else ()
            set (BUILD_VERSION ${CALC_RELEASE_VERSION})
            message ("calculated BUILD_VERSION ${BUILD_VERSION}")
         endif ()
      else ()
         message (FATAL_ERROR
            "BUILD_VERSION not specified and could not be calculated\
 (Python was not found on the system); specify in CMake command, -DBUILD_VERSION=<version>"
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

# Enable "maintainer flags," which are supplementary but not mandatory.
# (As opposed to MongoC-Warnings.cmake, which are required warnings)
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

if (NOT ENABLE_STATIC MATCHES "^(ON|OFF|AUTO|BUILD_ONLY|DONT_INSTALL)$")
   message (FATAL_ERROR
      "ENABLE_STATIC option must be ON, OFF, AUTO, BUILD_ONLY, or DONT_INSTALL"
   )
endif ()

set (MONGOC_ENABLE_STATIC_BUILD 0)
set (MONGOC_ENABLE_STATIC_INSTALL 0)

if (ENABLE_STATIC MATCHES "^(ON|AUTO)$")
   message (STATUS "Build and install static libraries")
   set (MONGOC_ENABLE_STATIC_BUILD 1)
   set (MONGOC_ENABLE_STATIC_INSTALL 1)
elseif (ENABLE_STATIC MATCHES "^(BUILD_ONLY|DONT_INSTALL)$")
   message (STATUS "Build only static libraries")
   set (MONGOC_ENABLE_STATIC_BUILD 1)
else ()
   message (STATUS "Don't build static libraries")
endif()

if (ENABLE_BSON STREQUAL SYSTEM)
   # The input variable BSON_ROOT_DIR is respected for backwards compatibility,
   # but you should use the standard CMAKE_PREFIX_PATH instead.
   message (STATUS "Searching for libbson CMake packages")
   find_package (bson-1.0
      "${MONGOC_MAJOR_VERSION}.${MONGOC_MINOR_VERSION}.${MONGOC_MICRO_VERSION}"
      HINTS
      ${BSON_ROOT_DIR})

   if (NOT bson-1.0_FOUND)
      message (FATAL_ERROR "System libbson not found")
   endif ()

   message ("--   libbson found version \"${bson-1.0_VERSION}\"")
   message ("--   disabling test-libmongoc since using system libbson")
   SET (ENABLE_TESTS OFF)

   set (USING_SYSTEM_BSON TRUE)
   if (NOT TARGET mongo::bson_shared)
           message (FATAL_ERROR "System libbson built without shared library target")
   endif ()
   set (BSON_LIBRARIES mongo::bson_shared)
   if (NOT TARGET mongo::bson_static)
           message (FATAL_ERROR "System libbson built without static library target")
   endif ()
   set (BSON_STATIC_LIBRARIES mongo::bson_static)
endif ()

unset (dist_generated CACHE)
unset (dist_generated_depends CACHE)

set (BUILD_SOURCE_DIR ${CMAKE_BINARY_DIR})

include (MakeDistFiles)

# Enable CTest
include (CTest)

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

# https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
# Enable features from POSIX.1-2008 as well as certain BSD and SVID features.
add_definitions (-D_DEFAULT_SOURCE)
list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_DEFAULT_SOURCE)

add_subdirectory (src/common)

if (NOT USING_SYSTEM_BSON)
   message ("  -- Using bundled libbson")

   add_subdirectory (src/libbson)
   # Defined in src/libbson/CMakeLists.txt
   set (BSON_STATIC_LIBRARIES bson_static)
   set (BSON_LIBRARIES bson_shared)
   set (BSON_STATIC_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/libbson/src" "${PROJECT_BINARY_DIR}/src/libbson/src")
   set (BSON_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/libbson/src" "${PROJECT_BINARY_DIR}/src/libbson/src")
   set (BSON_STATIC_PUBLIC_DEFINITIONS "BSON_STATIC")
endif ()

if (MSVC)
   add_definitions (-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (ENABLE_MONGOC)

   if (ENABLE_TESTS AND NOT MONGOC_ENABLE_STATIC_BUILD)
      message (FATAL_ERROR "ENABLE_TESTS requires ENABLE_STATIC or ENABLE_STATIC_BUILD")
   endif ()

   if (NOT ENABLE_SSL MATCHES "DARWIN|WINDOWS|OPENSSL|LIBRESSL|AUTO|OFF")
      message (FATAL_ERROR
               "ENABLE_SSL option must be DARWIN, WINDOWS, OPENSSL, LIBRESSL, AUTO, or OFF")
   endif ()

   set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

   set (ZLIB_SOURCES
      ${SOURCE_DIR}/src/zlib-1.2.12/adler32.c
      ${SOURCE_DIR}/src/zlib-1.2.12/crc32.c
      ${SOURCE_DIR}/src/zlib-1.2.12/deflate.c
      ${SOURCE_DIR}/src/zlib-1.2.12/infback.c
      ${SOURCE_DIR}/src/zlib-1.2.12/inffast.c
      ${SOURCE_DIR}/src/zlib-1.2.12/inflate.c
      ${SOURCE_DIR}/src/zlib-1.2.12/inftrees.c
      ${SOURCE_DIR}/src/zlib-1.2.12/trees.c
      ${SOURCE_DIR}/src/zlib-1.2.12/zutil.c
      ${SOURCE_DIR}/src/zlib-1.2.12/compress.c
      ${SOURCE_DIR}/src/zlib-1.2.12/uncompr.c
      ${SOURCE_DIR}/src/zlib-1.2.12/gzclose.c
      ${SOURCE_DIR}/src/zlib-1.2.12/gzlib.c
      ${SOURCE_DIR}/src/zlib-1.2.12/gzread.c
      ${SOURCE_DIR}/src/zlib-1.2.12/gzwrite.c
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

   set (KMS_MSG_SOURCES
      ${SOURCE_DIR}/src/kms-message/src/hexlify.c
      ${SOURCE_DIR}/src/kms-message/src/kms_b64.c
      ${SOURCE_DIR}/src/kms-message/src/kms_caller_identity_request.c
      ${SOURCE_DIR}/src/kms-message/src/kms_crypto_apple.c
      ${SOURCE_DIR}/src/kms-message/src/kms_crypto_libcrypto.c
      ${SOURCE_DIR}/src/kms-message/src/kms_crypto_none.c
      ${SOURCE_DIR}/src/kms-message/src/kms_crypto_windows.c
      ${SOURCE_DIR}/src/kms-message/src/kms_decrypt_request.c
      ${SOURCE_DIR}/src/kms-message/src/kms_encrypt_request.c
      ${SOURCE_DIR}/src/kms-message/src/kms_kv_list.c
      ${SOURCE_DIR}/src/kms-message/src/kms_message.c
      ${SOURCE_DIR}/src/kms-message/src/kms_port.c
      ${SOURCE_DIR}/src/kms-message/src/kms_request.c
      ${SOURCE_DIR}/src/kms-message/src/kms_request_opt.c
      ${SOURCE_DIR}/src/kms-message/src/kms_request_str.c
      ${SOURCE_DIR}/src/kms-message/src/kms_response.c
      ${SOURCE_DIR}/src/kms-message/src/kms_response_parser.c
      ${SOURCE_DIR}/src/kms-message/src/sort.c
   )

   if (NOT ENABLE_MONGODB_AWS_AUTH MATCHES "ON|OFF|AUTO")
      message (FATAL_ERROR "ENABLE_MONGODB_AWS_AUTH option must be ON, AUTO, or OFF")
   endif ()
   set (MONGOC_ENABLE_MONGODB_AWS_AUTH 0)
   if (ENABLE_MONGODB_AWS_AUTH STREQUAL ON)
      if (MSVC AND MSVC_VERSION LESS 1900)
         message (FATAL_ERROR "Use Visual Studio 2015 or higher for ENABLE_MONGODB_AWS_AUTH")
      endif ()
      if (ENABLE_SSL STREQUAL OFF)
         message (FATAL_ERROR
            "Option ENABLE_MONGODB_AWS_AUTH requires ENABLE_SSL not set to OFF"
         )
      endif ()
      set (MONGOC_ENABLE_MONGODB_AWS_AUTH 1)
   elseif (ENABLE_MONGODB_AWS_AUTH STREQUAL AUTO)
      if (MSVC AND MSVC_VERSION LESS 1900)
         message (WARNING "MS Visual Studio too old for ENABLE_MONGODB_AWS_AUTH")
      elseif (ENABLE_SSL STREQUAL OFF)
              message (WARNING
            "Option ENABLE_MONGODB_AWS_AUTH requires ENABLE_SSL not set to OFF"
         )
      else ()
         set (MONGOC_ENABLE_MONGODB_AWS_AUTH 1)
      endif ()
   endif ()

   add_subdirectory (src/libmongoc)

   if (MONGOC_ENABLE_MONGODB_AWS_AUTH)
      message (STATUS "Building with MONGODB-AWS auth support")
   endif ()

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
   # 'src/kms-message' was already included if appropriate
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

# Spit out some information regarding the generated build system
message (STATUS "Build files generated for:")
message (STATUS "\tbuild system: ${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR_INSTANCE)
   message (STATUS "\tinstance: ${CMAKE_GENERATOR_INSTANCE}")
endif ()
if (CMAKE_GENERATOR_PLATFORM)
   message (STATUS "\tinstance: ${CMAKE_GENERATOR_PLATFORM}")
endif ()
if (CMAKE_GENERATOR_TOOLSET)
   message (STATUS "\tinstance: ${CMAKE_GENERATOR_TOOLSET}")
endif ()

if (TARGET test-libmongoc)
   # Generate a file that can be included by CTest to load and enumerate all of the
   # tests defined by the test-libmongoc executable. Generate one for each
   # configuration in case of multiconf generators.
   string (CONFIGURE [=[
      set (TEST_LIBMONGOC_EXE [[$<TARGET_FILE:test-libmongoc>]])
      set (SRC_ROOT [[@PROJECT_SOURCE_DIR@]])
      set (IS_MULTICONF $<BOOL:@CMAKE_CONFIGURATION_TYPES@>)
      if (NOT IS_MULTICONF OR CTEST_CONFIGURATION_TYPE STREQUAL "$<CONFIG>")
         # We are not in multi-conf, or the current config matches our config.
         include ("${SRC_ROOT}/build/cmake/LoadTests.cmake")
      elseif (NOT CTEST_CONFIGURATION_TYPE)
         # We are in multi-conf, but no '-C' config was specified
         message (WARNING "Specify a --build-config when using CTest with a multi-config build")
      else ()
         # Do nothing. Not our config.
      endif ()
   ]=] code @ONLY)
   file (GENERATE
         OUTPUT "${PROJECT_BINARY_DIR}/LoadTests-$<CONFIG>.cmake"
         CONTENT "${code}")
   if (CMAKE_CONFIGURATION_TYPES)
      foreach (conf IN LISTS CMAKE_CONFIGURATION_TYPES)
         # Direct the generated CTest code to include() the file that loads the tests:
         set_property (
            DIRECTORY
            APPEND PROPERTY
            TEST_INCLUDE_FILES "${PROJECT_BINARY_DIR}/LoadTests-${conf}.cmake")
      endforeach ()
   else ()
      set_property (
         DIRECTORY
         APPEND PROPERTY
         TEST_INCLUDE_FILES "${PROJECT_BINARY_DIR}/LoadTests-${CMAKE_BUILD_TYPE}.cmake")
   endif ()
endif ()
