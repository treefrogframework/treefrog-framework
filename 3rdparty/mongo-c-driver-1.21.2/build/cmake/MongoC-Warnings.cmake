#[[
   This file sets warning options for the directories in which it is include()'d

   These warnings are intended to be ported to each supported platform, and
   especially for high-value warnings that are very likely to catch latent bugs
   early in the process before the code is even run.
]]

set (__is_gnu "$<C_COMPILER_ID:GNU>")
set (__is_clang "$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>>")
set (__is_gnu_like "$<OR:${__is_gnu},${__is_clang}>")
set (__is_msvc "$<C_COMPILER_ID:MSVC>")

# "Old" GNU is GCC < 5, which is missing several warning options
set (__is_old_gnu "$<AND:${__is_gnu},$<VERSION_LESS:$<C_COMPILER_VERSION>,5>>")
set (__not_old_gnu "$<NOT:${__is_old_gnu}>")

#[[
   Define additional compile options, conditional on the compiler being used.
   Each option should be prefixed by `gnu:`, `clang:`, `msvc:`, or `gnu-like:`.
   Those options will be conditionally enabled for GCC, Clang, or MSVC.

   These options are attached to the source directory and its children.
]]
function (mongoc_add_platform_compile_options)
   foreach (opt IN LISTS ARGV)
      if (NOT opt MATCHES "^(gnu-like|gnu|clang|msvc):(.*)")
         message (SEND_ERROR "Invalid option '${opt}' (Should be prefixed by 'msvc:', 'gnu:', 'clang:', or 'gnu-like:'")
         continue ()
      endif ()
      if (CMAKE_MATCH_1 STREQUAL "gnu-like")
         add_compile_options ("$<${__is_gnu_like}:${CMAKE_MATCH_2}>")
      elseif (CMAKE_MATCH_1 STREQUAL gnu)
         add_compile_options ("$<${__is_gnu}:${CMAKE_MATCH_2}>")
      elseif (CMAKE_MATCH_1 STREQUAL clang)
         add_compile_options ("$<${__is_clang}:${CMAKE_MATCH_2}>")
      elseif (CMAKE_MATCH_1 STREQUAL "msvc")
         add_compile_options ("$<${__is_msvc}:${CMAKE_MATCH_2}>")
      else ()
         message (SEND_ERROR "Invalid option to mongoc_add_platform_compile_options(): '${opt}'")
      endif ()
   endforeach ()
endfunction ()

if (CMAKE_VERSION VERSION_LESS 3.3)
   # On older CMake versions, we'll just always pass the warning options, even
   # if the generate warnings for the C++ check file
   set (is_c_lang "1")
else ()
   # $<COMPILE_LANGUAGE> is only valid in CMake 3.3+
   set (is_c_lang "$<COMPILE_LANGUAGE:C>")
endif ()

# These below warnings should always be unconditional hard errors, as the code is
# almost definitely broken
mongoc_add_platform_compile_options (
     # Implicit function or variable declarations
     gnu-like:$<${is_c_lang}:-Werror=implicit> msvc:/we4013 msvc:/we4431
     # Missing return types/statements
     gnu-like:-Werror=return-type msvc:/we4716
     # Incompatible pointer types
     gnu-like:$<$<AND:${is_c_lang},${__not_old_gnu}>:-Werror=incompatible-pointer-types> msvc:/we4113
     # Integral/pointer conversions
     gnu-like:$<$<AND:${is_c_lang},${__not_old_gnu}>:-Werror=int-conversion> msvc:/we4047
     # Discarding qualifiers
     gnu:$<$<AND:${is_c_lang},${__not_old_gnu}>:-Werror=discarded-qualifiers>
     clang:$<${is_c_lang}:-Werror=ignored-qualifiers>
     msvc:/we4090
     # Definite use of uninitialized value
     gnu-like:-Werror=uninitialized msvc:/we4700

     # Aside: Disable CRT insecurity warnings
     msvc:/D_CRT_SECURE_NO_WARNINGS
     )
