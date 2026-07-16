# This script may be executed in script mode by the uninstall target.
cmake_policy(VERSION 3.15...4.0)

# Avoid CMake Issue 26678: https://gitlab.kitware.com/cmake/cmake/-/issues/26678
if("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.27")
    cmake_policy(SET CMP0147 OLD)
endif()

if(NOT CMAKE_SCRIPT_MODE_FILE)
    # We are being included from within a project, so we should generate the install rules
    # The script name is "uninstall" by default:
    if(NOT DEFINED UNINSTALL_SCRIPT_NAME)
        set(UNINSTALL_SCRIPT_NAME "uninstall")
    endif()
    # We need a directory where we should install the script:
    if(NOT UNINSTALL_PROG_DIR)
        message(SEND_ERROR "We require an UNINSTALL_PROG_DIR to be defined")
    endif()
    # Platform dependent values:
    if(WIN32)
        set(_script_ext "cmd")
    else()
        set(_script_ext "sh")
    endif()
    # The script filename and path:
    set(_script_filename "${UNINSTALL_SCRIPT_NAME}.${_script_ext}")
    get_filename_component(_uninstaller_script "${CMAKE_CURRENT_BINARY_DIR}/${_script_filename}" ABSOLUTE)
    # Code that will do the work at install-time:
    string(CONFIGURE [==[
    function(__generate_uninstall)
        set(UNINSTALL_IS_WIN32 "@WIN32@")
        set(UNINSTALL_WRITE_FILE "@_uninstaller_script@")
        set(UNINSTALL_SCRIPT_SELF "@UNINSTALL_PROG_DIR@/@_script_filename@")
        include("@CMAKE_CURRENT_LIST_FILE@")
    endfunction()
    __generate_uninstall()
    ]==] code @ONLY ESCAPE_QUOTES)
    install(CODE "${code}")
    # Add a rule to install that file:
    install(
        FILES "${_uninstaller_script}"
        DESTINATION "${UNINSTALL_PROG_DIR}"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        )

    # If applicable, generate an "uninstall" target to run the uninstaller:
    if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR OR PROJECT_IS_TOP_LEVEL)
        add_custom_target(
            uninstall
            COMMAND "${_uninstaller_script}"
            COMMENT Uninstalling...
        )
    endif()
    # Stop here: The rest of the file is for install-time
    return()
endif()

if(NOT DEFINED CMAKE_INSTALL_MANIFEST_FILES)
    message(FATAL_ERROR "This file is only for use with CMake's install(CODE/SCRIPT) command")
endif()
if(NOT DEFINED UNINSTALL_WRITE_FILE)
    message(FATAL_ERROR "Expected a variable “UNINSTALL_WRITE_FILE” to be defined")
endif()

# Lock the uninstall file to synchronize with parallel install processes.
file(LOCK "${UNINSTALL_WRITE_FILE}.lock" GUARD PROCESS RESULT_VARIABLE lockres)
# Clear out the uninstall script before we begin writing:
file(WRITE "${UNINSTALL_WRITE_FILE}" "")

# Append a line to the uninstall script file. Single quotes will be replaced with doubles,
# and an appropriate newline will be added.
function(append_line line)
    string(REPLACE "'" "\"" line "${line}")
    file(APPEND "${UNINSTALL_WRITE_FILE}" "${line}\n")
endfunction()

# Ensure generated uninstall script has executable permissions.
if ("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.19.0")
    file (
        CHMOD "${UNINSTALL_WRITE_FILE}"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
else ()
    # Workaround lack of file(CHMOD).
    get_filename_component(_script_filename "${UNINSTALL_WRITE_FILE}" NAME)
    file (
        COPY "${UNINSTALL_WRITE_FILE}"
        DESTINATION "${_script_filename}.d"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
    file (RENAME "${_script_filename}.d/${_script_filename}" "${_script_filename}")
endif ()

# The copyright header:
set(header [[
MongoDB C Driver uninstall program, generated with CMake

Copyright 2009-present MongoDB, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.]])
string(STRIP header "${header}")
string(REPLACE "\n" ";" header_lines "${header}")

# Prefix for the shell script:
set(sh_preamble [[
set -eu

__rmfile() {
    set -eu
    abs=$__prefix/$1
    printf "Remove file %s: " "$abs"
    if test -f "$abs" || test -L "$abs"
    then
        rm -- "$abs"
        echo "ok"
    else
        echo "skipped: not present"
    fi
}

__rmdir() {
    set -eu
    abs=$__prefix/$1
    printf "Remove directory %s: " "$abs"
    if test -d "$abs"
    then
        list="$(find "$abs" -mindepth 1)"
        if test "$list" = ""
        then
            rmdir -- "$abs" 2>/dev/null && echo "ok" || echo "skipped: not empty"
        fi
    else
        echo "skipped: not present"
    fi
}
]])

# Convert the install prefix to an absolute path with the native path format:
get_filename_component(install_prefix "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
file(TO_NATIVE_PATH "${install_prefix}" install_prefix)
# Handling DESTDIR requires careful handling of root path redirection:
set(root_path)
set(relative_prefix "${install_prefix}")
if(COMMAND cmake_path)
    cmake_path(GET install_prefix ROOT_PATH root_path)
    cmake_path(GET install_prefix RELATIVE_PART relative_prefix)
endif()

# The first lines that will be written to the script:
set(init_lines)

if(UNINSTALL_IS_WIN32)
    # Comment the header:
    list(TRANSFORM header_lines PREPEND "rem " REGEX "^.+$")
    list(TRANSFORM header_lines PREPEND "rem" REGEX "^$")
    # Add the preamble
    list(APPEND init_lines
        "@echo off"
        ""
        "${header_lines}"
        ""
        "if \"%DESTDIR%\"==\"\" (set __prefix=${install_prefix}) else (set __prefix=!DESTDIR!\\${relative_prefix})"
        ""
        "(GOTO) 2>nul & (")
else()
    # Comment the header:
    list(TRANSFORM header_lines PREPEND "# " REGEX "^.+$")
    list(TRANSFORM header_lines PREPEND "#" REGEX "^$")
    # Add the preamble
    list(APPEND init_lines
        "#!/usr/bin/env bash"
        "#"
        "${header_lines}"
        ""
        "${sh_preamble}"
        "__prefix=\${DESTDIR:-}${install_prefix}"
        "")
endif()

# Add the first lines to the file:
string(REPLACE ";" "\n" init "${init_lines}")
append_line("${init}")

# Generate a "remove a file" command
function(add_rmfile filename)
    file(TO_NATIVE_PATH "${filename}" native)
    if(WIN32)
      set(file "%__prefix%\\${native}")
      set(rmfile_lines
        "  <nul set /p \"=Remove file: ${file} \""
        "  if EXIST \"${file}\" ("
        "    del /Q /F \"${file}\" && echo - ok"
        "  ) ELSE echo - skipped: not present"
        ") && (")
      string(REPLACE ";" "\n" rmfile "${rmfile_lines}")
      append_line("${rmfile}")
    else()
      append_line("__rmfile '${native}'")
    endif()
endfunction()

# Generate a "remove a directory" command
function(add_rmdir dirname)
    file(TO_NATIVE_PATH "${dirname}" native)
    if(WIN32)
      set(dir "%__prefix%\\${native}")
      set(rmdir_lines
        "  <nul set /p \"=Remove directory: ${dir} \""
        "  if EXIST \"${dir}\" ("
        "    rmdir /Q \"${dir}\" 2>nul && echo - ok || echo - skipped ^(non-empty?^)"
        "  ) ELSE echo - skipped: not present"
        ") && (")
      string(REPLACE ";" "\n" rmdir "${rmdir_lines}")
      append_line("${rmdir}")
    else()
      append_line("__rmdir '${native}'")
    endif()
endfunction()

set(script_self "${install_prefix}/${UNINSTALL_SCRIPT_SELF}")
set(dirs_to_remove)
foreach(installed IN LISTS CMAKE_INSTALL_MANIFEST_FILES script_self)
    # Get the relative path from the prefix (the uninstaller will fix it up later)
    file(RELATIVE_PATH relpath "${install_prefix}" "${installed}")
    # Add a removal:
    add_rmfile("${relpath}")
    # Climb the path and collect directories:
    while("1")
        get_filename_component(installed "${installed}" DIRECTORY)
        file(TO_NATIVE_PATH "${installed}" installed)
        get_filename_component(parent "${installed}" DIRECTORY)
        file(TO_NATIVE_PATH "${parent}" parent)
        # Don't account for the prefix or direct children of the prefix:
        if(installed STREQUAL install_prefix OR parent STREQUAL install_prefix)
            break()
        endif()
        # Keep track of this directory for later:
        list(APPEND dirs_to_remove "${installed}")
    endwhile()
endforeach()

# Now generate commands to remove (empty) directories:
list(REMOVE_DUPLICATES dirs_to_remove)
# Order them by depth so that we remove subdirectories before their parents:
list(SORT dirs_to_remove ORDER DESCENDING)
foreach(dir IN LISTS dirs_to_remove)
    file(RELATIVE_PATH relpath "${install_prefix}" "${dir}")
    add_rmdir("${relpath}")
endforeach()

# Allow the batch script delete itself without error.
if(WIN32)
    append_line("  ver>nul")
    append_line(")")
endif()

message(STATUS "Generated uninstaller: ${UNINSTALL_WRITE_FILE}")
