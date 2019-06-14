cmake_minimum_required (VERSION 3.1)

project (libmongoc C)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/../../build/cmake)

include (InstallRequiredSystemLibraries)

message ("libmongoc version (from VERSION_CURRENT file): ${MONGOC_VERSION}")

# Defaults.
set (MONGOC_ENABLE_SSL 0)
set (MONGOC_ENABLE_SSL_OPENSSL 0)
set (MONGOC_HAVE_ASN1_STRING_GET0_DATA 0)
set (MONGOC_ENABLE_SSL_LIBRESSL 0)
set (MONGOC_ENABLE_SSL_SECURE_TRANSPORT 0)
set (MONGOC_ENABLE_SSL_SECURE_CHANNEL 0)

set (MONGOC_ENABLE_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)

set (MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE 0)

set (MONGOC_ENABLE_COMPRESSION 0)
set (MONGOC_ENABLE_COMPRESSION_SNAPPY 0)
set (MONGOC_ENABLE_COMPRESSION_ZLIB 0)

if (ENABLE_COVERAGE)
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g --coverage")
endif ()

if (NOT ENABLE_ZLIB MATCHES "SYSTEM|AUTO|BUNDLED|OFF")
   message (FATAL_ERROR
      "ENABLE_ZLIB option must be SYSTEM, BUNDLED, AUTO, or OFF"
      )
endif ()

# Disable warnings on bundled zlib source files.
set_source_files_properties (${ZLIB_SOURCES} PROPERTIES COMPILE_FLAGS -w)

# Copy zconf.h.in to zconf.h; even when using system zlib, the 'dist' target
# will look for zconf.h in that location.
configure_file (
   "${SOURCE_DIR}/src/zlib-1.2.11/zconf.h.in"
   "${CMAKE_BINARY_DIR}/src/zlib-1.2.11/zconf.h"
   COPYONLY
)
if (ENABLE_ZLIB MATCHES "SYSTEM|AUTO")
   message (STATUS "Searching for zlib CMake packages")
   include (FindZLIB)
   if (ZLIB_FOUND)
      message ("--   zlib found version \"${ZLIB_VERSION_STRING}\"")
      message ("--   zlib include path \"${ZLIB_INCLUDE_DIRS}\"")
      message ("--   zlib libraries \"${ZLIB_LIBRARIES}\"")
      include_directories ("${ZLIB_INCLUDE_DIRS}")
   else ()
      if (ENABLE_ZLIB STREQUAL "SYSTEM")
         message (FATAL_ERROR
            "Unable to find system zlib package. Either specify the zlib \
            location by setting ZLIB_ROOT, or else set ENABLE_ZLIB=BUNDLED or \
            set ENABLE_ZLIB=OFF."
            )
      endif ()
      set (ZLIB_LIBRARIES "")
   endif ()
endif ()

if ( (ENABLE_ZLIB STREQUAL "BUNDLED")
   OR (ENABLE_ZLIB STREQUAL "AUTO" AND NOT ZLIB_FOUND) )
   message (STATUS "Enabling zlib compression (bundled)")
   set (SOURCES ${SOURCES} ${ZLIB_SOURCES})

   list (
      APPEND
      MONGOC_INTERNAL_INCLUDE_DIRS
      "${SOURCE_DIR}/src/zlib-1.2.11"
      "${CMAKE_BINARY_DIR}/src/zlib-1.2.11"
   )
endif ()

if (NOT ENABLE_ZLIB STREQUAL "OFF")
   # At this point the system zlib was found, or the bundled library was used
   include (CheckIncludeFiles)
   check_include_files ("unistd.h" HAVE_UNISTD_H)
   check_include_files ("stdarg.h" HAVE_STDARG_H)
   set (MONGOC_ENABLE_COMPRESSION 1)
   set (MONGOC_ENABLE_COMPRESSION_ZLIB 1)
else ()
   message (STATUS "Disabling zlib compression")
endif ()

if (NOT ENABLE_SSL STREQUAL OFF)
   # Try OpenSSL automatically everywhere but Mac and Windows.
   if (ENABLE_SSL STREQUAL "OPENSSL"
       OR (NOT APPLE AND NOT WIN32 AND ENABLE_SSL STREQUAL "AUTO"))
      # Sets OPENSSL_FOUND on success.
      include (FindOpenSSL)
   endif ()

   if (ENABLE_SSL STREQUAL LIBRESSL)
      include (FindPkgConfig)
      message ("-- Searching for LibreSSL/libtls")
      pkg_check_modules (LIBRESSL libtls)
      if (LIBRESSL_FOUND)
         message ("--   Found ${LIBRESSL_LIBRARIES}")
         set (SSL_LIBRARIES ${LIBRESSL_LIBRARIES})
         if (LIBRESSL_INCLUDE_DIRS)
           include_directories ("${LIBRESSL_INCLUDE_DIRS}")
         endif ()
         link_directories ("${LIBRESSL_LIBRARY_DIRS}")
         set (LIBRESSL 1)
      else ()
         message ("--   Not found")
      endif ()
   endif ()

   if (ENABLE_SSL STREQUAL DARWIN OR (APPLE AND ENABLE_SSL STREQUAL "AUTO"))
      if (APPLE)
         set (SECURE_TRANSPORT 1)
      else ()
         message (FATAL_ERROR "ENABLE_SSL=DARWIN only supported on Mac OS X")
      endif ()
   endif ()

   if (ENABLE_SSL STREQUAL WINDOWS OR (WIN32 AND ENABLE_SSL STREQUAL "AUTO"))
      if (WIN32)
         set (SECURE_CHANNEL 1)
      else ()
         message (FATAL_ERROR "ENABLE_SSL=WINDOWS only supported on Windows")
      endif ()
   endif ()

   if (NOT OPENSSL_FOUND AND NOT SECURE_TRANSPORT AND NOT SECURE_CHANNEL AND NOT LIBRESSL)
      if (ENABLE_SSL STREQUAL AUTO)
         set (ENABLE_SSL OFF)
      else ()
         message (FATAL_ERROR "No SSL library found")
      endif ()
   endif ()
endif ()

if (OPENSSL_FOUND)
   if (WIN32 AND OPENSSL_VERSION GREATER 1.1 AND NOT
         ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.7)
      message (FATAL_ERROR "Building against OpenSSL 1.1.0 and later requires CMake 3.7 or later (hint:"
         " You can also compile against Windows Secure Transport with -DENABLE_SSL=WINDOWS")
   endif ()
   if (APPLE AND NOT OPENSSL_ROOT_DIR)
      message (WARNING "Building with OpenSSL but OPENSSL_ROOT_DIR not defined. If build fails to link"
         " to OpenSSL, define OPENSSL_ROOT_DIR as the path to the OpenSSL installation directory.")
   endif ()
   include (CheckLibraryExists)
   # Check for newer OpenSSL string function.
   check_library_exists ("${OPENSSL_CRYPTO_LIBRARY}"
      ASN1_STRING_get0_data "openssl/asn1.h" HAVE_ASN1_STRING_GET0_DATA
   )
   if (HAVE_ASN1_STRING_GET0_DATA)
      set (MONGOC_HAVE_ASN1_STRING_GET0_DATA 1)
   endif ()
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_OPENSSL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
elseif (SECURE_TRANSPORT)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_SECURE_TRANSPORT 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
elseif (SECURE_CHANNEL)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_SECURE_CHANNEL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_CNG 1)
elseif (LIBRESSL)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_LIBRESSL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
endif ()

if (ENABLE_CRYPTO_SYSTEM_PROFILE)
   if (OPENSSL_FOUND)
      set (MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE 1)
   else ()
      message (FATAL_ERROR "ENABLE_CRYPTO_SYSTEM_PROFILE only available with OpenSSL")
   endif ()
endif ()

if (NOT ENABLE_SASL MATCHES "CYRUS|SSPI|AUTO|OFF")
   message (FATAL_ERROR
      "ENABLE_SASL option must be CYRUS, SSPI, AUTO, or OFF")
endif ()

# Defaults.
set (MONGOC_ENABLE_SASL 0)
set (MONGOC_ENABLE_SASL_CYRUS 0)
set (MONGOC_ENABLE_SASL_SSPI 0)
set (MONGOC_HAVE_SASL_CLIENT_DONE 0)

if (NOT ENABLE_SASL STREQUAL OFF)
   if ( (ENABLE_SASL MATCHES "SSPI|AUTO") AND WIN32)
      set (MONGOC_ENABLE_SASL 1)
      set (MONGOC_ENABLE_SASL_SSPI 1)
   elseif (ENABLE_SASL MATCHES "AUTO|CYRUS")
      # Sets SASL_LIBRARIES.
      include (FindSASL2)
      if (SASL_FOUND)
         set (MONGOC_ENABLE_SASL 1)
         set (MONGOC_ENABLE_SASL_CYRUS 1)
      elseif (ENABLE_SASL STREQUAL "CYRUS")
          message (FATAL_ERROR "ENABLE_SASL=CYRUS could not be satisfied")
      endif()
   endif ()
endif ()


if (ENABLE_AUTOMATIC_INIT_AND_CLEANUP)
   set (MONGOC_NO_AUTOMATIC_GLOBALS 0)
else ()
   set (MONGOC_NO_AUTOMATIC_GLOBALS 1)
endif ()

include (CheckTypeSize)
if (WIN32)
   SET (CMAKE_EXTRA_INCLUDE_FILES "ws2tcpip.h")
else ()
   SET (CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
endif ()
CHECK_TYPE_SIZE (socklen_t HAVE_SOCKLEN)
SET (CMAKE_EXTRA_INCLUDE_FILES)

if (HAVE_SOCKLEN)
   set (MONGOC_HAVE_SOCKLEN 1)
   set (MONGOC_SOCKET_ARG3 "socklen_t")
else ()
   set (MONGOC_HAVE_SOCKLEN 0)
   set (MONGOC_SOCKET_ARG3 "int")
endif ()

include (FindResSearch)
include (CheckSchedGetCPU)

function (mongoc_get_accept_args ARG2 ARG3)
   SET (VAR 0)
   foreach (ARG2_VAL "struct sockaddr" "void")
   foreach (ARG3_VAL "socklen_t" "size_t" "int")

   MATH (EXPR VAR "${VAR}+1")

   FILE (WRITE ${CMAKE_CURRENT_BINARY_DIR}/accept_test${VAR}.c
   "#include <sys/types.h>
    #include <sys/socket.h>

    int main ()
    {
      int a = 0;
      ${ARG2_VAL} *b = 0;
      ${ARG3_VAL} *c = 0;
      accept (a, b, c);
      return 0;
   }
   ")

   TRY_COMPILE (RES ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}/accept_test${VAR}.c CMAKE_FLAGS
   "-Werror -DCMAKE_CXX_LINK_EXECUTABLE='echo not linking now...'" OUTPUT_VARIABLE LOG2)

   if (RES)
      message (
         STATUS
         "Detected parameters: accept (int, ${ARG2_VAL} *, ${ARG3_VAL} *)")

      set (${ARG2} ${ARG2_VAL} PARENT_SCOPE)
      set (${ARG3} ${ARG3_VAL} PARENT_SCOPE)
      return ()
   endif ()

   endforeach ()
   endforeach ()

endfunction ()

# Reasonable defaults.
set (MONGOC_SOCKET_ARG2 "struct sockaddr")
set (MONGOC_SOCKET_ARG3 "socklen_t")

if (NOT WIN32)
   mongoc_get_accept_args (MONGOC_SOCKET_ARG2 MONGOC_SOCKET_ARG3)
endif ()

set (MONGOC_API_VERSION 1.0)

set (CPACK_PACKAGE_VERSION_MAJOR ${MONGOC_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${MONGOC_MINOR_VERSION})

set (MONGOC_CC ${CMAKE_C_COMPILER})
set (MONGOC_USER_SET_CFLAGS ${CMAKE_C_FLAGS})
set (MONGOC_USER_SET_LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})

set (MONGOC_TRACE 0)

if (ENABLE_TRACING)
   set (MONGOC_TRACE 1)
endif ()

# Sets SNAPPY_LIBRARIES and SNAPPY_INCLUDE_DIRS.
include (FindSnappy)
if (SNAPPY_INCLUDE_DIRS)
   set (MONGOC_ENABLE_COMPRESSION 1)
   include_directories ("${SNAPPY_INCLUDE_DIRS}")
endif ()

set (MONGOC_ENABLE_SHM_COUNTERS 0)

if (NOT ENABLE_SHM_COUNTERS MATCHES "ON|OFF|AUTO")
   message (FATAL_ERROR "ENABLE_SHM_COUNTERS option must be ON, OFF, or AUTO")
endif ()

if (ENABLE_SHM_COUNTERS STREQUAL "AUTO")
   if (UNIX AND NOT APPLE)
      set (ENABLE_SHM_COUNTERS ON)
   endif ()
endif ()

if (ENABLE_SHM_COUNTERS STREQUAL "ON")
   if (APPLE OR NOT UNIX)
      message (
         FATAL_ERROR
         "Shared memory performance counters not supported on Mac or Windows")
   endif ()
   set (MONGOC_ENABLE_SHM_COUNTERS 1)
   find_library(RT_LIBRARY rt HINTS /usr/lib32)
   if (RT_LIBRARY)
      set (SHM_LIBRARIES rt)
   endif ()
endif ()

if (NOT ENABLE_ICU MATCHES "AUTO|ON|OFF")
   message (FATAL_ERROR, "ENABLE_ICU option must be AUTO, ON, or OFF")
endif()

if (NOT ENABLE_ICU STREQUAL OFF)
   if (ENABLE_ICU STREQUAL ON)
      # do not suppress log output if find_package cannot find ICU
      find_package (ICU COMPONENTS uc)
   elseif (ENABLE_ICU STREQUAL AUTO)
      find_package (ICU QUIET COMPONENTS uc)
   endif()
   if (ICU_FOUND)
      set (MONGOC_ENABLE_ICU 1)
      include_directories ("${ICU_INCLUDE_DIR}")
   elseif (ENABLE_ICU STREQUAL ON)
      message (FATAL_ERROR "No ICU library found. If ICU is installed in a non-standard directory, define ICU_ROOT as the ICU installation path.")
   elseif (ENABLE_ICU STREQUAL AUTO)
      message (STATUS "No ICU library found, SASLPrep disabled for SCRAM-SHA-256 authentication.")
      message (STATUS "If ICU is installed in a non-standard directory, define ICU_ROOT as the ICU installation path.")
   endif()
endif()

configure_file (
   "${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-config.h.in"
   "${PROJECT_BINARY_DIR}/src/mongoc/mongoc-config.h"
)

configure_file (
   "${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-version.h.in"
   "${PROJECT_BINARY_DIR}/src/mongoc/mongoc-version.h"
)

if (ENABLE_APPLE_FRAMEWORK)
   configure_file (
      "${PROJECT_SOURCE_DIR}/src/mongoc/modules/module.modulemap.in"
      "${PROJECT_BINARY_DIR}/src/mongoc/modules/module.modulemap"
   )
endif ()

include_directories ("${PROJECT_BINARY_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/../../src/common")

set (SOURCES ${SOURCES}
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-apm.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-array.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-async.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-async-cmd.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-buffer.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-bulk-operation.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-change-stream.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client-pool.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cluster.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cluster-sasl.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-collection.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-compression.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-counters.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-array.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-cmd.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-cmd-deprecated.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-find.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-find-cmd.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-find-opquery.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-legacy.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor-array.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-database.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-error.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-find-and-modify.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-init.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-bucket.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-bucket-file.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-page.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-handshake.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-host-list.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-index.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-init.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-list.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-linux-distro-scanner.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-log.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-matcher.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-matcher-op.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-memcmp.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cmd.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-opts.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-opts-helpers.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-queue.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-read-concern.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-read-prefs.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rpc.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-server-description.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-server-stream.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client-session.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-set.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-socket.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-file.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs-download.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs-upload.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-socket.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-topology.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-topology-description.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-topology-description-apm.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-topology-scanner.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-uri.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-util.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-version-functions.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-write-command.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-write-command-legacy.c
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-write-concern.c
   ${PROJECT_SOURCE_DIR}/../../src/common/common-b64.c
   ${PROJECT_SOURCE_DIR}/../../src/common/common-md5.c
)

set (HEADERS
   ${PROJECT_BINARY_DIR}/src/mongoc/mongoc-config.h
   ${PROJECT_BINARY_DIR}/src/mongoc/mongoc-version.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-apm.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-bulk-operation.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-change-stream.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client-pool.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-collection.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cursor.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-database.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-error.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-flags.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-find-and-modify.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-bucket.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-page.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-handshake.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-host-list.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-init.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-index.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-iovec.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-log.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-macros.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-matcher.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-opcode.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-prelude.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-read-concern.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-read-prefs.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-server-description.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-client-session.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-socket.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-libressl.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-file.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-socket.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-topology-description.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-uri.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-version-functions.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-write-concern.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rand.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls.h
   ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-ssl.h
)

set (HEADERS_FORWARDING
   ${PROJECT_SOURCE_DIR}/src/mongoc/forwarding/mongoc.h
)

if (NOT ENABLE_SSL STREQUAL OFF)
   set (SOURCES ${SOURCES}
      ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-crypto.c
      ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-scram.c
      ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls.c
      ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-ssl.c
   )

   if (OPENSSL_FOUND)
      message (STATUS "Compiling against OpenSSL")
      set (SOURCES ${SOURCES}
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-crypto-openssl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rand-openssl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl-bio.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-openssl.c
      )
      set (SSL_LIBRARIES ${OPENSSL_LIBRARIES})
      include_directories (${OPENSSL_INCLUDE_DIR})
      if (WIN32)
         set (SSL_LIBRARIES ${SSL_LIBRARIES} crypt32.lib)
      endif ()
   elseif (SECURE_TRANSPORT)
      message (STATUS "Compiling against Secure Transport")
      set (SOURCES ${SOURCES}
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-crypto-common-crypto.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rand-common-crypto.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-secure-transport.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-secure-transport.c
      )
      set (SSL_LIBRARIES "-framework CoreFoundation -framework Security")
   elseif (SECURE_CHANNEL)
      message (STATUS "Compiling against Secure Channel")
      set (SOURCES ${SOURCES}
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-crypto-cng.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rand-cng.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-secure-channel.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-secure-channel.c
      )
      set (SSL_LIBRARIES secur32.lib crypt32.lib Bcrypt.lib)
   elseif (LIBRESSL)
      message (STATUS "Compiling against LibreSSL")
      set (SOURCES ${SOURCES}
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-crypto-openssl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-rand-openssl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-stream-tls-libressl.c
         ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-libressl.c
      )
      set (SSL_LIBRARIES -ltls -lcrypto)
   endif ()
else ()
   message (STATUS "SSL disabled")
endif () # ENABLE_SSL

if (MONGOC_ENABLE_SASL)
   set (SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-sasl.c)
   if (MONGOC_ENABLE_SASL_CYRUS)
      message (STATUS "Compiling against Cyrus SASL")
      set (SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cluster-cyrus.c)
      set (SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cyrus.c)
      include_directories (${SASL_INCLUDE_DIRS})
   elseif (MONGOC_ENABLE_SASL_SSPI)
      message (STATUS "Compiling against Windows SSPI")
      set (SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-cluster-sspi.c)
      set (SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/mongoc/mongoc-sspi.c)
      set (SASL_LIBRARIES secur32.lib crypt32.lib Shlwapi.lib)
   endif ()
else ()
   message (STATUS "SASL disabled")
endif ()


set (THREADS_PREFER_PTHREAD_FLAG 1)
find_package (Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
   set (THREAD_LIB ${CMAKE_THREAD_LIBS_INIT})
endif ()

set (LIBRARIES
   ${SASL_LIBRARIES} ${SSL_LIBRARIES} ${SHM_LIBRARIES} ${RESOLV_LIBRARIES}
   ${SNAPPY_LIBRARIES} ${ZLIB_LIBRARIES} Threads::Threads ${ICU_LIBRARIES}
)

if (WIN32)
   set (LIBRARIES ${LIBRARIES} ws2_32)
endif ()

add_library (mongoc_shared SHARED ${SOURCES} ${HEADERS} ${HEADERS_FORWARDING})
set_target_properties (mongoc_shared PROPERTIES CMAKE_CXX_VISIBILITY_PRESET hidden)
target_link_libraries (mongoc_shared ${LIBRARIES} ${BSON_LIBRARIES})
target_include_directories (mongoc_shared BEFORE PUBLIC ${BSON_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
target_compile_definitions (mongoc_shared PUBLIC MONGOC_COMPILATION ${BSON_DEFINITIONS})

set_target_properties (mongoc_shared PROPERTIES VERSION 0.0.0 SOVERSION 0)
set_target_properties (mongoc_shared PROPERTIES OUTPUT_NAME "mongoc-${MONGOC_API_VERSION}" PREFIX "lib")

if (MONGOC_ENABLE_STATIC)
   add_library (mongoc_static STATIC ${SOURCES} ${HEADERS} ${HEADERS_FORWARDING})
   target_link_libraries (mongoc_static ${LIBRARIES} ${BSON_STATIC_LIBRARIES})
   target_include_directories (mongoc_static BEFORE PUBLIC ${BSON_STATIC_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
   target_compile_definitions (mongoc_static PUBLIC MONGOC_COMPILATION MONGOC_STATIC ${BSON_STATIC_DEFINITIONS})
   set_target_properties (mongoc_static PROPERTIES VERSION 0.0.0)
   set_target_properties (mongoc_static PROPERTIES OUTPUT_NAME "mongoc-static-${MONGOC_API_VERSION}")
endif ()

if (ENABLE_APPLE_FRAMEWORK)
   set_target_properties (mongoc_shared PROPERTIES 
      FRAMEWORK TRUE 
      MACOSX_FRAMEWORK_BUNDLE_VERSION ${MONGOC_VERSION} 
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MONGOC_VERSION}
      MACOSX_FRAMEWORK_IDENTIFIER org.mongodb.mongoc
      OUTPUT_NAME "mongoc"
      PUBLIC_HEADER "${HEADERS}" 
   )
endif ()

add_executable (mongoc-stat ${PROJECT_SOURCE_DIR}/../../src/tools/mongoc-stat.c)
target_link_libraries (mongoc-stat mongoc_shared)

# mongoc-stat works if shared memory performance counters are enabled.
if (ENABLE_SHM_COUNTERS STREQUAL "ON")
   install (PROGRAMS ${PROJECT_BINARY_DIR}/mongoc-stat
      DESTINATION ${CMAKE_INSTALL_BINDIR}
   )
endif ()

function (mongoc_add_test test use_shared)
   if (ENABLE_TESTS)
      add_executable (${test} ${ARGN})
      if (NOT MSVC)
         # We've tests that test our deprecated api. MSVC 2013 will complain about invalid flag
         set_source_files_properties (${ARGN} PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
      endif ()
      target_include_directories (${test} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
      if (${use_shared})
         target_link_libraries (${test} mongoc_shared)
         target_include_directories (${test} PRIVATE ${BSON_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
      else ()
         target_compile_definitions (${test} PUBLIC "MONGOC_COMPILATION" PUBLIC "BSON_COMPILATION")
         target_link_libraries (${test} mongoc_static)
         target_include_directories (${test} PRIVATE ${BSON_STATIC_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
      endif ()
      target_link_libraries (${test} ${RESOLV_LIBRARIES})
      if (WIN32)
         target_link_libraries (${test} shlwapi)
      else ()
         target_link_libraries (${test})
      endif ()
   endif ()
endfunction ()

function (mongoc_add_example example use_shared)
   if (ENABLE_EXAMPLES)
      add_executable (${example} ${ARGN})
      if (${use_shared})
         target_link_libraries (${example} mongoc_shared)
      else ()
         target_link_libraries (${example} mongoc_static)
      endif ()
      if (WIN32)
         target_link_libraries (${example} shlwapi)
      endif ()
      set (EXAMPLES ${EXAMPLES} ${example})
   endif ()
endfunction ()

set (test-libmongoc-sources
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/corpus-test.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/corpus-test.h
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-atomic.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bson.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bson-corpus.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bson-error.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bson-version.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-endian.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-clock.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-decimal128.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-iso8601.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-iter.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-json.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-oid.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-reader.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-string.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-utf8.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-value.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-writer.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bcon-basic.c
   ${PROJECT_SOURCE_DIR}/../../src/libbson/tests/test-bcon-extract.c
   ${PROJECT_SOURCE_DIR}/tests/debug-stream.c
   ${PROJECT_SOURCE_DIR}/tests/json-test.c
   ${PROJECT_SOURCE_DIR}/tests/json-test-monitoring.c
   ${PROJECT_SOURCE_DIR}/tests/json-test-operations.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/future.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/future-functions.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/future-value.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/sync-queue.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/mock-rs.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/mock-server.c
   ${PROJECT_SOURCE_DIR}/tests/mock_server/request.c
   ${PROJECT_SOURCE_DIR}/tests/test-conveniences.c
   ${PROJECT_SOURCE_DIR}/tests/test-libmongoc.c
   ${PROJECT_SOURCE_DIR}/tests/test-happy-eyeballs.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-array.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-async.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-buffer.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-bulk.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-change-stream.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-client.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-client-pool.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-cluster.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-collection.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-collection-find.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-collection-find-with-opts.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-connection-uri.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-command-monitoring.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-counters.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-crud.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-cursor.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-database.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-error.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-exhaust.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-find-and-modify.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-gridfs.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-gridfs-bucket.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-gridfs-file-page.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-handshake.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-linux-distro-scanner.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-list.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-log.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-matcher.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-max-staleness.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-opts.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-queue.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-read-concern.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-read-write-concern.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-read-prefs.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-retryable-writes.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-rpc.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-sample-commands.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-scram.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-sdam.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-sdam-monitoring.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-server-selection.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-server-selection-errors.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-transactions.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-client-session.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-set.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-socket.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-dns.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-stream.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-thread.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-topology.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-topology-description.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-topology-reconcile.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-topology-scanner.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-uri.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-version.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-usleep.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-util.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-write-commands.c
   ${PROJECT_SOURCE_DIR}/tests/test-mongoc-write-concern.c
   ${PROJECT_SOURCE_DIR}/tests/TestSuite.c
)

if (MONGOC_ENABLE_SSL)
   set (test-libmongoc-sources ${test-libmongoc-sources}
      ${PROJECT_SOURCE_DIR}/tests/test-mongoc-x509.c
      ${PROJECT_SOURCE_DIR}/tests/ssl-test.c
      ${PROJECT_SOURCE_DIR}/tests/test-mongoc-stream-tls.c
      ${PROJECT_SOURCE_DIR}/tests/test-mongoc-stream-tls-error.c
   )
endif ()

if (MONGOC_ENABLE_SASL_CYRUS)
   set (test-libmongoc-sources ${test-libmongoc-sources}
      ${PROJECT_SOURCE_DIR}/tests/test-mongoc-cyrus.c
   )
endif ()

mongoc_add_test (test-libmongoc FALSE ${test-libmongoc-sources})
mongoc_add_test (test-mongoc-gssapi TRUE ${PROJECT_SOURCE_DIR}/tests/test-mongoc-gssapi.c)

if (ENABLE_TESTS)
   enable_testing ()
   add_test (NAME test-libmongoc
      COMMAND test-libmongoc
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../..
   )

   # "make test" doesn't compile tests, so we create "make check" which compiles
   # and runs tests: https://gitlab.kitware.com/cmake/cmake/issues/8774
   add_custom_target (check COMMAND ${CMAKE_CTEST_COMMAND} -V
      DEPENDS test-libmongoc
   )
endif ()

# examples/
mongoc_add_example (example-client TRUE ${PROJECT_SOURCE_DIR}/examples/example-client.c)
mongoc_add_example (example-collection-watch TRUE ${PROJECT_SOURCE_DIR}/examples/example-collection-watch.c)
if (NOT WIN32)
   mongoc_add_example (example-resume TRUE ${PROJECT_SOURCE_DIR}/examples/example-resume.c)
endif()
mongoc_add_example (example-start-at-optime TRUE ${PROJECT_SOURCE_DIR}/examples/example-start-at-optime.c)
mongoc_add_example (example-command-monitoring TRUE ${PROJECT_SOURCE_DIR}/examples/example-command-monitoring.c)
mongoc_add_example (example-command-with-opts TRUE ${PROJECT_SOURCE_DIR}/examples/example-command-with-opts.c)
mongoc_add_example (example-create-indexes TRUE ${PROJECT_SOURCE_DIR}/examples/example-create-indexes.c)
mongoc_add_example (example-gridfs TRUE ${PROJECT_SOURCE_DIR}/examples/example-gridfs.c)
mongoc_add_example (example-gridfs-bucket TRUE ${PROJECT_SOURCE_DIR}/examples/example-gridfs-bucket.c)
if (NOT WIN32 AND ENABLE_EXAMPLES)
   mongoc_add_example (example-pool TRUE ${PROJECT_SOURCE_DIR}/examples/example-pool.c)
   target_link_libraries (example-pool Threads::Threads)
endif ()
mongoc_add_example (example-scram TRUE ${PROJECT_SOURCE_DIR}/examples/example-scram.c)
mongoc_add_example (example-sdam-monitoring TRUE ${PROJECT_SOURCE_DIR}/examples/example-sdam-monitoring.c)
mongoc_add_example (example-session TRUE ${PROJECT_SOURCE_DIR}/examples/example-session.c)
mongoc_add_example (example-transaction TRUE ${PROJECT_SOURCE_DIR}/examples/example-transaction.c)
mongoc_add_example (example-update TRUE ${PROJECT_SOURCE_DIR}/examples/example-update.c)
mongoc_add_example (find-and-modify TRUE ${PROJECT_SOURCE_DIR}/examples/find-and-modify.c)
mongoc_add_example (hello_mongoc TRUE ${PROJECT_SOURCE_DIR}/examples/hello_mongoc.c)
mongoc_add_example (mongoc-dump TRUE ${PROJECT_SOURCE_DIR}/examples/mongoc-dump.c)
mongoc_add_example (mongoc-ping TRUE ${PROJECT_SOURCE_DIR}/examples/mongoc-ping.c)
mongoc_add_example (mongoc-tail TRUE ${PROJECT_SOURCE_DIR}/examples/mongoc-tail.c)

# examples/aggregation/
mongoc_add_example (aggregation1 TRUE ${PROJECT_SOURCE_DIR}/examples/aggregation/aggregation1.c)

# examples/basic_aggregation/
mongoc_add_example (basic-aggregation TRUE ${PROJECT_SOURCE_DIR}/examples/basic_aggregation/basic-aggregation.c)

# examples/bulk/
mongoc_add_example (bulk-collation TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk-collation.c)
mongoc_add_example (bulk1 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk1.c)
mongoc_add_example (bulk2 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk2.c)
mongoc_add_example (bulk3 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk3.c)
mongoc_add_example (bulk4 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk4.c)
mongoc_add_example (bulk5 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk5.c)
mongoc_add_example (bulk6 TRUE ${PROJECT_SOURCE_DIR}/examples/bulk/bulk6.c)

# examples/common_operations/
mongoc_add_example (common-operations TRUE ${PROJECT_SOURCE_DIR}/examples/common_operations/common-operations.c)

# examples/find_and_modify_with_opts/
mongoc_add_example (fam TRUE ${PROJECT_SOURCE_DIR}/examples/find_and_modify_with_opts/fam.c)

file (COPY ${PROJECT_SOURCE_DIR}/tests/binary DESTINATION ${PROJECT_BINARY_DIR}/tests)
file (COPY ${PROJECT_SOURCE_DIR}/tests/json DESTINATION ${PROJECT_BINARY_DIR}/tests)
file (COPY ${PROJECT_SOURCE_DIR}/tests/x509gen DESTINATION ${PROJECT_BINARY_DIR}/tests)
file (COPY ${PROJECT_SOURCE_DIR}/tests/release_files DESTINATION ${PROJECT_BINARY_DIR}/tests)

install (
   TARGETS mongoc_shared ${EXAMPLES}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} 
)

if (MONGOC_ENABLE_STATIC)
   install (
      TARGETS mongoc_shared mongoc_static ${EXAMPLES}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} 
   )
endif ()

set (MONGOC_HEADER_INSTALL_DIR
   "${CMAKE_INSTALL_INCLUDEDIR}/libmongoc-${MONGOC_API_VERSION}"
)

install (
   FILES ${HEADERS}
   DESTINATION "${MONGOC_HEADER_INSTALL_DIR}/mongoc"
)

install (
   FILES ${HEADERS_FORWARDING}
   DESTINATION "${MONGOC_HEADER_INSTALL_DIR}"
)

if (ENABLE_APPLE_FRAMEWORK)
   install (
      FILES "${PROJECT_BINARY_DIR}/src/mongoc/modules/module.modulemap"
      DESTINATION "${CMAKE_INSTALL_BINDIR}/mongoc.framework/Modules/"
   )
endif ()

# Define pkg-config files
set (VERSION "${MONGOC_VERSION}")
set (prefix "${CMAKE_INSTALL_PREFIX}")
set (libdir "\${prefix}/lib")

foreach (
      FLAG
      ${SASL_LIBRARIES} ${SSL_LIBRARIES} ${SHM_LIBRARIES} ${RESOLV_LIBRARIES}
      ${THREAD_LIB} ${ZLIB_LIBRARIES} ${SNAPPY_LIBRARIES} ${ICU_LIBRARIES})

   if (IS_ABSOLUTE "${FLAG}")
      get_filename_component (FLAG_DIR "${FLAG}" DIRECTORY)
      get_filename_component (FLAG_FILE "${FLAG}" NAME_WE)
      STRING (REGEX REPLACE "^lib" "" FLAG_FILE "${FLAG_FILE}")
      set (MONGOC_LIBRARIES "${MONGOC_LIBRARIES} -L${FLAG_DIR} -l${FLAG_FILE}")
   elseif (FLAG MATCHES "^-.*")
      # Flag starts with dash, add it as-is.
      set (MONGOC_LIBRARIES "${MONGOC_LIBRARIES} ${FLAG}")
   else ()
      # Flag doesn't start with dash, add it with a dash.
      set (MONGOC_LIBRARIES "${MONGOC_LIBRARIES} -l${FLAG}")
   endif ()
endforeach ()

configure_file (
   ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-1.0.pc.in
   ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-1.0.pc
@ONLY)
install (
   FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-1.0.pc
   DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
if (MONGOC_ENABLE_STATIC)
   configure_file (
      ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-static-1.0.pc.in
      ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-static-1.0.pc
   @ONLY)
   install (
      FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-static-1.0.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
   )
endif ()
# Deprecated alias for libmongoc-1.0.pc, see CDRIVER-2086.
if (MONGOC_ENABLE_SSL)
   configure_file (
      ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-ssl-1.0.pc.in
      ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-ssl-1.0.pc
   @ONLY)
   install (
      FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-ssl-1.0.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
   )
endif ()

include (../../build/cmake/MongoCPackage.cmake)

if (ENABLE_MAN_PAGES STREQUAL ON OR ENABLE_HTML_DOCS STREQUAL ON)
   find_package (Sphinx REQUIRED)
   add_subdirectory (doc)
   add_custom_target (mongoc-doc
      ALL
      DEPENDS
      $<$<STREQUAL:"${ENABLE_MAN_PAGES}","ON">:mongoc-man>
      $<$<STREQUAL:"${ENABLE_HTML_DOCS}","ON">:mongoc-html>
   )
endif ()

add_subdirectory (build)
# sub-directory 'doc' was already included above
add_subdirectory (examples)
add_subdirectory (src)
add_subdirectory (tests)

set_local_dist (src_libmongoc_DIST_local
   CMakeLists.txt
   THIRD_PARTY_NOTICES
)

set (src_libmongoc_DIST
   ${src_libmongoc_DIST_local}
   ${src_libmongoc_build_DIST}
   ${src_libmongoc_doc_DIST}
   ${src_libmongoc_examples_DIST}
   ${src_libmongoc_src_DIST}
   ${src_libmongoc_tests_DIST}
   PARENT_SCOPE
)
