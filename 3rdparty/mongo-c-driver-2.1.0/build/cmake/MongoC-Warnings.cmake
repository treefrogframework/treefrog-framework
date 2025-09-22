#[[
   This file sets warning options in the "mongoc-warning-options" variable when included.

   These warnings are intended to be ported to each supported platform, and
   especially for high-value warnings that are very likely to catch latent bugs
   early in the process before the code is even run.
]]

#[[
   Appends additional compile options to the "mongoc-warning-options" variable, conditioned
   on the compiler being used.

   Each option should be prefixed by `gnu:`, `clang:`, `msvc:`, or `gnu-like:`.
   Those options will be conditionally enabled for GCC, Clang, or MSVC.
]]
function (mongoc_add_warning_options)
   list(APPEND CMAKE_MESSAGE_CONTEXT ${CMAKE_CURRENT_FUNCTION})
   # Conditional prefixes:
   set(cond/gnu $<C_COMPILER_ID:GNU>)
   set(cond/llvm-clang $<C_COMPILER_ID:Clang>)
   set(cond/apple-clang $<C_COMPILER_ID:AppleClang>)
   set(cond/clang $<OR:${cond/llvm-clang},${cond/apple-clang}>)
   set(cond/gnu-like $<OR:${cond/gnu},${cond/clang}>)
   set(cond/msvc $<C_COMPILER_ID:MSVC>)
   set(cond/lang-c $<COMPILE_LANGUAGE:C>)
   # "Old" GNU is GCC < 5, which is missing several warning options
   set(cond/gcc-lt5 $<AND:${cond/gnu},$<VERSION_LESS:$<C_COMPILER_VERSION>,5>>)
   set(cond/gcc-lt7 $<AND:${cond/gnu},$<VERSION_LESS:$<C_COMPILER_VERSION>,7>>)
   set(cond/gcc-lt8 $<AND:${cond/gnu},$<VERSION_LESS:$<C_COMPILER_VERSION>,8>>)
   set(cond/gcc-lt11 $<AND:${cond/gnu},$<VERSION_LESS:$<C_COMPILER_VERSION>,11>>)
   set(cond/clang-lt4 $<AND:${cond/clang},$<VERSION_LESS:$<C_COMPILER_VERSION>,4>>)
   set(cond/clang-lt10 $<AND:${cond/clang},$<VERSION_LESS:$<C_COMPILER_VERSION>,10>>)
   set(cond/clang-lt19 $<AND:${cond/clang},$<VERSION_LESS:$<C_COMPILER_VERSION>,19>>)
   # Process options:
   foreach (opt IN LISTS ARGV)
      # Replace prefixes. Matches right-most first:
      while (opt MATCHES "(.*)(^|:)([a-z0-9-]+):(.*)")
         set(before "${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
         set(prefix "${CMAKE_MATCH_3}")
         set(suffix "${CMAKE_MATCH_4}")
         message(TRACE "Substitution: prefix “${prefix}” in “${opt}”, suffix is “${suffix}”")
         set(cond "cond/${prefix}")
         set(not 0)
         if(prefix MATCHES "^not-(.*)")
            set(cond "cond/${CMAKE_MATCH_1}")
            set(not 1)
         endif()
         if(DEFINED "${cond}")
            set(expr "${${cond}}")
            if(not)
               set(expr "$<NOT:${expr}>")
            endif()
            set(opt "$<${expr}:${suffix}>")
         else ()
            message (SEND_ERROR "Unknown option prefix to ${CMAKE_CURRENT_FUNCTION}(): “${prefix}” in “${opt}”")
            break()
         endif ()
         set(opt "${before}${opt}")
         message(TRACE "Become: ${opt}")
      endwhile ()
      list(APPEND mongoc-warning-options "${opt}")
   endforeach ()
   set(mongoc-warning-options "${mongoc-warning-options}" PARENT_SCOPE)
endfunction ()

set (is_c_lang "$<COMPILE_LANGUAGE:C>")

set (mongoc-warning-options "")

# These below warnings should always be unconditional hard errors, as the code is
# almost definitely broken
mongoc_add_warning_options (
     # Implicit function or variable declarations
     gnu-like:lang-c:-Werror=implicit msvc:/we4013 msvc:/we4431
     # Missing return types/statements
     gnu-like:-Werror=return-type msvc:/we4716
     # Incompatible pointer types
     gnu-like:lang-c:not-gcc-lt5:-Werror=incompatible-pointer-types msvc:/we4113
     # Integral/pointer conversions
     gnu-like:lang-c:not-gcc-lt5:-Werror=int-conversion msvc:/we4047
     # Discarding qualifiers
     gnu:lang-c:not-gcc-lt5:-Werror=discarded-qualifiers
     clang:lang-c:-Werror=ignored-qualifiers
     msvc:/we4090
     # Definite use of uninitialized value
     gnu-like:-Werror=uninitialized msvc:/we4700

     # Format strings.
     gnu-like:-Werror=format
     gnu-like:-Werror=format=2
     # GCC does not document the full list of warnings enabled by -Wformat.
     # For assurance, explicitly include those not listed by -Wformat or -Wformat=2.
     gnu:not-gcc-lt11:-Werror=format-diag
     gnu:not-gcc-lt8:-Werror=format-overflow=2
     gnu:not-gcc-lt5:-Werror=format-signedness
     gnu:not-gcc-lt8:-Werror=format-truncation=2
     # Clang does not include several flags in `-Wformat` or `-Wformat=2`.
     # For assurance, explicitly include those not listed by -Wformat or -Wformat=2.
     clang:-Werror=format-non-iso
     clang:not-clang-lt4:-Werror=format-pedantic
     clang:not-clang-lt19:-Werror=format-signedness
     clang:not-clang-lt10:-Werror=format-type-confusion

     # Aside: Disable CRT insecurity warnings
     msvc:/D_CRT_SECURE_NO_WARNINGS
     )
