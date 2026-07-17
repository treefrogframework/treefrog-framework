# sub-directory 'libbson' was already included at the top-level
# sub-directory 'libmongoc' was already included at the top-level
# sub-directory 'kms-message' was already included at the top-level

# Copy zconf.h.in to zconf.h, used by zlib
configure_file (
   "${mongo-c-driver_SOURCE_DIR}/src/zlib-1.3.1/zconf.h.in"
   "${CMAKE_BINARY_DIR}/src/zlib-1.3.1/zconf.h"
   COPYONLY
)

if (CMAKE_CXX_COMPILER)
   # Add a C++ source file that will #include the main C headers. This "library"
   # does nothing other than validate that the C headers are valid C++ headers.
   add_library (mongoc-cxx-check OBJECT cpp-check.cpp)
   if (TARGET mongoc::static)
      target_link_libraries (mongoc-cxx-check PRIVATE mongoc::static)
   else ()
      target_link_libraries (mongoc-cxx-check PRIVATE mongoc::shared)
   endif ()
endif ()

# public-header-warnings
add_executable(public-header-warnings EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/c-check.c)

if (TARGET mongoc::static)
   target_link_libraries(public-header-warnings PRIVATE mongoc::static)
else ()
   target_link_libraries(public-header-warnings PRIVATE mongoc::shared)
endif ()

target_compile_options(public-header-warnings PRIVATE
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Weverything>
  $<$<C_COMPILER_ID:GNU>:-Wextra>
  $<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Werror>
  $<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-declaration-after-statement>
  $<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-disabled-macro-expansion>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-c++98-compat-pedantic>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-pre-c11-compat>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-pre-c2x-compat>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-unsafe-buffer-usage>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-padded>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-reserved-identifier>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-documentation-unknown-command>
  $<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-unknown-warning-option>
  $<$<C_COMPILER_ID:AppleClang>:-Wno-poison-system-directories>
  $<$<C_COMPILER_ID:MSVC>:/W4>
  $<$<C_COMPILER_ID:MSVC>:/WX>
  $<$<C_COMPILER_ID:MSVC>:/wd4324>
)
