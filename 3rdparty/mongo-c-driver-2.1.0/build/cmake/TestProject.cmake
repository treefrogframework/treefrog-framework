#[[
Add a CMake test that configures, builds, and tests a CMake project.

    add_test_cmake_project(
        <name> <path>
        [INSTALL_PARENT]
        [BUILD_DIR <dir>]
        [GENERATOR <gen>]
        [CONFIG <config>]
        [PASSTHRU_VARS ...]
        [PASSTHRU_VARS_REGEX <regex>]
        [CONFIGURE_ARGS ...]
        [SETTINGS ...]
    )

The generated test will run CMake configure on the project at `<path>` (which
is resolved relative to the caller's source directory).

If INSTALL_PARENT is specified, then the host CMake project will be installed to a
temporary prefix, and that prefix will be passed along with CMAKE_PREFIX_PATH
when the test project is configured.

PASSTHRU_VARS is a list of CMake variables visible in the current scope that should
be made visible to the subproject with the same name and value. PASSTHRU_VARS_REGEX
takes a single regular expression. Any variables currently defined which match
the regex will be passed through as-if they were specified with PASSTHRU_VARS.

SETTINGS is list of a `[name]=[value]` strings for additional `-D` arguments to
pass to the sub-project. List arguments *are* supported.

If CONFIG is unspecified, then the generated test will configure+build the project
according to the configuration of CTest (passed with the `-C` argument).

The default for GENERATOR is to use the same generator as the host project.

The default BUILD_DIR will be a temporary directory that will be automatically
deleted at the start of each test to ensure a fresh configure+build cycle.

Additional Variables
####################

In addition to the variables specified explicitly in the call, all variables
with the suffix `_PATH` or `_DIR` will be passed to the sub-project with
`HOST_PROJECT_` prepended. For example, CMAKE_SOURCE_DIR will be passed to
the sub-project as HOST_PROJECT_CMAKE_SOURCE_DIR
]]
function(add_test_cmake_project name path)
    # Logging context
    list(APPEND CMAKE_MESSAGE_CONTEXT "${CMAKE_CURRENT_FUNCTION}")

    # Parse command arguments:
    set(options INSTALL_PARENT)
    set(args BUILD_DIR GENERATOR CONFIG PASSTHRU_VARS_REGEX)
    set(list_args SETTINGS PASSTHRU_VARS CONFIGURE_ARGS)
    cmake_parse_arguments(PARSE_ARGV 2 _tp_arg "${options}" "${args}" "${list_args}")
    foreach(unknown IN LISTS _tp_arg_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "Unknown argument: “${unknown}”")
    endforeach()
    if(_tp_arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Bad arguments (see above)")
    endif()

    # Default values:
    if(NOT DEFINED _tp_arg_BUILD_DIR)
        set(dirname "${name}")
        if(WIN32)
            # Escape specials
            string(REPLACE "%" "%25" dirname "${dirname}")
            string(REPLACE ":" "%3A" dirname "${dirname}")
            string(REPLACE "?" "%3F" dirname "${dirname}")
            string(REPLACE "*" "%2A" dirname "${dirname}")
            string(REPLACE "\"" "%22" dirname "${dirname}")
            string(REPLACE "\\" "%5C" dirname "${dirname}")
            string(REPLACE "<" "%3C" dirname "${dirname}")
            string(REPLACE ">" "%3E" dirname "${dirname}")
            string(REPLACE "|" "%7C" dirname "${dirname}")
        endif()
        set(_tp_arg_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/TestProject/${dirname}")
    endif()
    if(NOT DEFINED _tp_arg_GENERATOR)
        set(_tp_arg_GENERATOR "${CMAKE_GENERATOR}")
    endif()
    # Normalize paths
    if(NOT IS_ABSOLUTE "${_tp_arg_BUILD_DIR}")
        set(_tp_arg_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${_tp_arg_BUILD_DIR}")
    endif()
    get_filename_component(path "${path}" ABSOLUTE)
    message(VERBOSE "Add test project [${path}]")

    # Arguments that will be given during the CMake configure step:
    string(REPLACE ";" $<SEMICOLON> configure_args "${_tp_arg_CONFIGURE_ARGS}")

    # Build the argument lists that will be passed-through to project configuration -D flags:
    set(settings_passthru)

    # Pass through all _DIR and _PATH variables with a HOST_PROJECT_ prefix:
    get_directory_property(fwd_path_vars VARIABLES)
    list(FILTER fwd_path_vars INCLUDE REGEX "_DIR$|_PATH$")
    list(FILTER fwd_path_vars EXCLUDE REGEX "^_")
    list(SORT   fwd_path_vars CASE INSENSITIVE)
    set(dir_passthrough)
    foreach(var IN LISTS fwd_path_vars)
        string(REPLACE ";" $<SEMICOLON> value "${${var}}")
        list(APPEND settings_passthru "HOST_PROJECT_${var}=${value}")
    endforeach()

    # Pass through other variables without a prefix:
    set(passthru_vars "${_tp_arg_PASSTHRU_VARS}")
    # Some platform variables should always go through:
    list(APPEND passthru_vars
        CMAKE_C_COMPILER
        CMAKE_CXX_COMPILER
    )
    if(DEFINED _tp_arg_PASSTHRU_VARS_REGEX)
        # Pass through variables matching a certain pattern:
        get_directory_property(fwd_vars VARIABLES)
        list(FILTER fwd_vars INCLUDE REGEX "${_tp_arg_PASSTHRU_VARS_REGEX}")
        list(APPEND passthru_vars "${fwd_vars}")
    endif()

    # Pass through all variables that we've marked to be forwarded
    foreach(var IN LISTS passthru_vars)
        string(REPLACE ";" $<SEMICOLON> value "${${var}}")
        list(APPEND settings_passthru "${var}=${value}")
    endforeach()

    # Settings set with SETTINGS
    list(TRANSFORM _tp_arg_SETTINGS REPLACE ";" $<SEMICOLON> OUTPUT_VARIABLE settings_escaped)
    list(APPEND settings_passthru ${settings_escaped})

    # Add a prefix to each variable to mark it as a pass-thru variable:
    list(TRANSFORM settings_passthru PREPEND "-D;TEST_PROJECT_SETTINGS/")

    # Generate the test case:
    add_test(
        NAME "${name}"
        COMMAND ${CMAKE_COMMAND}
            --log-context
            -D "TEST_PROJECT_NAME=${name}"
            -D "TEST_PROJECT_PARENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -D "TEST_PROJECT_SOURCE_DIR=${path}"
            -D "TEST_PROJECT_BINARY_DIR=${_tp_arg_BUILD_DIR}"
            -D "TEST_PROJECT_GENERATOR=${_tp_arg_GENERATOR}"
            -D TEST_PROJECT_INSTALL_PARENT=${_tp_arg_INSTALL_PARENT}
            -D "TEST_PROJECT_CONFIGURE_ARGS=${_tp_arg_CONFIGURE_ARGS}"
            -D "TEST_PROJECT_CONFIG=$<CONFIG>"
            ${settings_passthru}
            -D __test_project_run=1
            -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
    )
endfunction()

# This function implements the actual test.
function(__do_test_project)
    list(APPEND CMAKE_MESSAGE_CONTEXT "TestProject Execution")
    cmake_path(ABSOLUTE_PATH TEST_PROJECT_SOURCE_DIR NORMALIZE OUTPUT_VARIABLE src_dir)
    cmake_path(ABSOLUTE_PATH TEST_PROJECT_BINARY_DIR NORMALIZE OUTPUT_VARIABLE bin_dir)

    string(MD5 test_name_hash "${TEST_PROJECT_NAME}")
    set(tmp_install_prefix "${TEST_PROJECT_PARENT_BINARY_DIR}/TestProject-install/${test_name_hash}")
    file(REMOVE_RECURSE "${tmp_install_prefix}")
    list(APPEND TEST_PROJECT_SETTINGS/CMAKE_INSTALL_PREFIX "${tmp_install_prefix}")
    list(APPEND TEST_PROJECT_SETTINGS/CMAKE_PREFIX_PATH "${tmp_install_prefix}")

    if(TEST_PROJECT_INSTALL_PARENT)
        cmake_path(ABSOLUTE_PATH tmp_install_prefix NORMALIZE)
        message(STATUS "Installing parent project into [${tmp_install_prefix}]")
        execute_process(
            COMMAND
                # Suppress DESTDIR
                ${CMAKE_COMMAND} -E env --unset=DESTDIR
                # Do the install:
                ${CMAKE_COMMAND}
                    --install "${TEST_PROJECT_PARENT_BINARY_DIR}"
                    --prefix "${tmp_install_prefix}"
                    --config "${TEST_PROJECT_CONFIG}"
            COMMAND_ERROR_IS_FATAL LAST
        )
    endif()
    message(STATUS "Project source dir: [${src_dir}]")
    message(STATUS "Project build dir: [${bin_dir}]")
    message(STATUS "Deleting build directory …")
    file(REMOVE_RECURSE "${bin_dir}")
    file(MAKE_DIRECTORY "${bin_dir}")

    # Grab settings passed-through from the parent project:
    get_directory_property(vars VARIABLES)
    set(fwd_settings)
    list(FILTER vars INCLUDE REGEX "^TEST_PROJECT_SETTINGS/")
    if(vars)
        message(STATUS "Configuration settings:")
    endif()
    foreach(var IN LISTS vars)
        set(value "${${var}}")
        # Remove our prefix
        string(REGEX REPLACE "^TEST_PROJECT_SETTINGS/" "" varname "${var}")
        # Print the value we received for debugging purposes
        message(STATUS "  • ${varname}=${value}")
        # Escape nested lists
        string(REPLACE ";" "\\;" value "${value}")
        list(APPEND fwd_settings -D "${varname}=${value}")
    endforeach()

    message(STATUS "Configuring project [${src_dir}] …")
    set(config_log "${bin_dir}/TestProject-configure.log")
    message(STATUS "CMake configure output will be written to [${config_log}]")
    execute_process(
        COMMAND_ECHO STDERR
        WORKING_DIRECTORY "${bin_dir}"
        RESULT_VARIABLE retc
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        COMMAND ${CMAKE_COMMAND}
            -S "${src_dir}"
            -B "${bin_dir}"
            -G "${TEST_PROJECT_GENERATOR}"
            -D "CMAKE_BUILD_TYPE=${TEST_PROJECT_BUILD_TYPE}"
            --no-warn-unused-cli
            --log-context
            --log-level=debug
            ${fwd_settings}
            ${TEST_PROJECT_CONFIGURE_ARGS}
    )
    file(WRITE "${config_log}" "${output}")
    if(retc)
        message(FATAL_ERROR "Configure subcommand failed [${retc}]")
    endif()
    message(STATUS "CMake configure completed")

    set(build_log "${bin_dir}/TestProject-build.log")
    message(STATUS "Build output will be written to [${build_log}]")
    message(STATUS "Building configured project [${bin_dir}] …")
    execute_process(
        COMMAND_ECHO STDERR
        WORKING_DIRECTORY "${bin_dir}"
        RESULT_VARIABLE retc
        OUTPUT_VARIABLE out
        ERROR_VARIABLE out
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        COMMAND ${CMAKE_COMMAND}
            --build "${bin_dir}"
            --config "${TEST_PROJECT_CONFIG}"
    )
    file(WRITE "${build_log}" "${output}")
    if(retc)
        message(FATAL_ERROR "Project build failed [${retc}]")
    endif()
    message(STATUS "Project build completed")

    execute_process(
        COMMAND ${CMAKE_CTEST_COMMAND}
            -T Start -T Test
            -C "${TEST_PROJECT_CONFIG}"
            --output-on-failure
        WORKING_DIRECTORY "${bin_dir}"
        COMMAND_ERROR_IS_FATAL LAST
    )
endfunction()

if(__test_project_run)
    cmake_minimum_required(VERSION 3.20)  # cmake_path/execute_process
    __do_test_project()
endif()
