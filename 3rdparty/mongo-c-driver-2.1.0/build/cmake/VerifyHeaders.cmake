include_guard(DIRECTORY)

#[==[
Add header verification targets for given headers:

    mongo_verify_headers(
        <tag>
        [USES_LIBRARIES [<library> ...]]
        [HEADERS [<glob> ...]]
        [EXCLUDE_REGEX [<pattern> ...]]
    )

Here `<tag>` is an arbitrary string that is used to qualify the internal target
created for the verification. The `<glob>` expressions are used to automatically
collect sources files (relative to the current source directory). All files
collected by `<glob>` must live within the current source directory.

After collecting sources according to the `<glob>` patterns, sources are
excluded if the filepath contains any substring that matches any regular
expression in the `<pattern>` list. Each `<pattern>` is tested against the
relative path to the header file that was found by `<glob>`, and not the
absolute path to the file.

The header verification targets are compiled according to the usage requirements
from all `<library>` arguments.
]==]
function(mongo_verify_headers tag)
    list(APPEND CMAKE_MESSAGE_CONTEXT "${CMAKE_CURRENT_FUNCTION}(${tag})")
    cmake_parse_arguments(
        PARSE_ARGV 1 arg
        ""  # No flags
        ""  # No args
        "HEADERS;EXCLUDE_REGEX;USE_LIBRARIES"  # List args
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # Collect headers according to our patterns
    set(headers_to_verify)
    foreach(pattern IN LISTS arg_HEADERS)
        # Use a recursive glob from the current source dir:
        file(GLOB_RECURSE more
            # Make the paths relative to the calling dir to prevent parent paths
            # from interfering with the exclusion regex logic below
            RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
            # We need to re-run configuration if any files are added/removed
            CONFIGURE_DEPENDS
            "${pattern}"
            )
        # Warn if this pattern didn't match anything. It is probably a mistake
        # in the caller's argument list.
        if(NOT more)
            message(WARNING "Globbing pattern “${pattern}” did not match any files")
        endif()
        list(APPEND headers_to_verify ${more})
    endforeach()

    # Exclude anything that matches any exclusion regex
    foreach(pattern IN LISTS arg_EXCLUDE_REGEX)
        list(FILTER headers_to_verify EXCLUDE REGEX "${pattern}")
    endforeach()

    # Drop duplicates since globs may grab a file more than once
    list(REMOVE_DUPLICATES headers_to_verify)
    list(SORT headers_to_verify)
    foreach(file IN LISTS headers_to_verify)
        message(DEBUG "Verify header file: ${file}")
    endforeach()

    # We create two targets: One for C and one for C++
    # C target
    set(c_target ${tag}-verify-headers-c)
    message(DEBUG "Defining header verification target “${c_target}” (C)")
    # Create object libraries. They will only have one empty compiled source file.
    # The source file language will tell CMake how to verify the associated header files.
    add_library(${c_target} OBJECT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/empty.c")
    # Define the file set
    target_sources(${c_target} PUBLIC FILE_SET HEADERS)
    # Conditionally do the same thing for C++
    if(CMAKE_CXX_COMPILER)
        # C++ is available. define it
        set(cxx_target ${tag}-verify-headers-cxx)
        message(DEBUG "Defining header verification targets “${cxx_target}” (C++)")
        add_library(${cxx_target} OBJECT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/empty.cpp")
        target_sources(${cxx_target} PUBLIC FILE_SET HEADERS)
    else()
        message(AUTHOR_WARNING "No C++ compiler is available, so the header-check C++ targets won't be defined")
        unset(cxx_target)
    endif()
    # Populate the properties and file sets.
    set_target_properties(${c_target} ${cxx_target} PROPERTIES
        # The main header file set:
        HEADER_SET "${headers_to_verify}"
        # Enable header verification:
        VERIFY_INTERFACE_HEADER_SETS TRUE
        # Add the usage requirements that propagate to the generated compilation rules:
        INTERFACE_LINK_LIBRARIES "${arg_USE_LIBRARIES}"
        )
endfunction()

#[[
Variable set to TRUE if-and-only-if CMake supports header verification.
]]
set(MONGO_CAN_VERIFY_HEADERS FALSE)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    set(MONGO_CAN_VERIFY_HEADERS TRUE)
endif()

# Try to enable C++, but don't require it. This will be used to conditionally
# define the C++ header-check tests
include(CheckLanguage)
check_language(CXX)
if(CMAKE_CXX_COMPILER)
    enable_language(CXX)
endif()
