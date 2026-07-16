include_guard(GLOBAL)

#[[
    Attempts to find the current build version string by reading VERSION_CURRENT
    from the current source directory.

    The computed build version is set in the parent scope according to `outvar`.
]]
function(compute_build_version outvar)
    list(APPEND CMAKE_MESSAGE_CONTEXT ${CMAKE_CURRENT_FUNCTION})
    # If it is present, defer to the VERSION_CURRENT file:
    set(ver_cur_file "${CMAKE_CURRENT_SOURCE_DIR}/VERSION_CURRENT")
    message(DEBUG "Using existing VERSION_CURRENT file as BUILD_VERSION [${ver_cur_file}]")
    file(READ "${ver_cur_file}" version)
    message(DEBUG "VERSION_CURRENT is “${version}”")
    set("${outvar}" "${version}" PARENT_SCOPE)
endfunction()

# Compute the BUILD_VERSION if it is not already defined:
if(NOT DEFINED BUILD_VERSION)
    compute_build_version(BUILD_VERSION)
endif()

# Set a BUILD_VERSION_SIMPLE, which is just a three-number-triple that CMake understands
string (REGEX REPLACE "([0-9]+\\.[0-9]+\\.[0-9]+).*$" "\\1" BUILD_VERSION_SIMPLE "${BUILD_VERSION}")
