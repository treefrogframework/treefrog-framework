#[[
    CMake package file for the BSON library.

    This file globs and includes all "*-targets.cmake" files in the containing
    directory, and intends that those files define the actual libbson targets.

    This file also defines a `bson::bson` target, which redirects to either
    `bson::static` or `bson::shared` depending on what type of library is
    available and can be controlled with an import-time CMake setting.

    If the installation has a static library, it is named `bson::static`. If
    the installation has a shared (dynamic) library, it is named `bson::shared`.
]]

# Check for missing components before proceeding. We don't provide any, so we
# should generate an error if the caller requests any *required* components.
set(missing_required_components)
foreach(comp IN LISTS bson_FIND_COMPONENTS)
    if(bson_FIND_REQUIRED_${comp})
        list(APPEND missing_required_components "${comp}")
    endif()
endforeach()

if(missing_required_components)
    list(JOIN missing_required_components ", " components)
    set(bson_FOUND FALSE)
    set(bson_NOT_FOUND_MESSAGE "The package version is compatible, but is missing required components: ${components}")
    # Stop now. Don't generate any imported targets
    return()
endif()

include(CMakeFindDependencyMacro)
find_dependency(Threads)  # Required for Threads::Threads

# Import the target files that will be installed alongside this file. Only the
# targets of libraries that were actually installed alongside this file will be imported
file(GLOB __targets_files "${CMAKE_CURRENT_LIST_DIR}/*-targets.cmake")
foreach(__file IN LISTS __targets_files)
    include("${__file}")
endforeach()

# The library type that is linked with `bson::bson`
set(__default_lib_type SHARED)
if(TARGET bson::static)
    # If static is available, set it as the default library type
    set(__default_lib_type STATIC)
endif()

# Allow the user to tweak what library type is linked for `bson::bson`
set(BSON_DEFAULT_IMPORTED_LIBRARY_TYPE "${__default_lib_type}"
    CACHE STRING "The default library type that is used when linking against 'bson::bson' (either SHARED or STATIC, requires that the package was built with the appropriate library type)")
set_property(CACHE BSON_DEFAULT_IMPORTED_LIBRARY_TYPE PROPERTY STRINGS SHARED STATIC)

if(NOT TARGET bson::bson)  # Don't redefine the target if we were already included
    string(TOLOWER "${BSON_DEFAULT_IMPORTED_LIBRARY_TYPE}" __type)
    add_library(bson::bson IMPORTED INTERFACE)
    set_property(TARGET bson::bson APPEND PROPERTY INTERFACE_LINK_LIBRARIES bson::${__type})
endif()
