include (CheckCSourceCompiles)
include (CMakePushCheckState)
include (MongoSettings)

mongo_setting (
   MONGO_SANITIZE "Semicolon/comma-separated list of sanitizers to apply when building"
   DEFAULT
      DEVEL EVAL [[
         if(NOT MSVC)
            set(DEFAULT "address,undefined")
         endif()
      ]])

mongo_bool_setting(
    MONGO_FUZZ "Enable libFuzzer integration (Requires a C++ compiler)"
    DEFAULT VALUE OFF
    VALIDATE CODE [[
        if (MONGO_FUZZ AND NOT ENABLE_STATIC)
            message (FATAL_ERROR "MONGO_FUZZ requires ENABLE_STATIC=ON or ENABLE_STATIC=BUILD_ONLY")
        endif ()
    ]]
)

if (MONGO_FUZZ)
    set(mongo_fuzz_options "address,undefined,fuzzer-no-link")
    if (MONGO_SANITIZE AND NOT "${MONGO_SANITIZE}" STREQUAL "${mongo_fuzz_options}")
        message(WARNING "Overriding user-provided MONGO_SANITIZE options due to MONGO_FUZZ=ON")
    endif ()
    set_property (CACHE MONGO_SANITIZE PROPERTY VALUE "${mongo_fuzz_options}")
endif ()

# Replace commas with semicolons for the genex
string(REPLACE ";" "," _sanitize "${MONGO_SANITIZE}")

if (_sanitize)
    string (MAKE_C_IDENTIFIER "HAVE_SANITIZE_${_sanitize}" ident)
    string (TOUPPER "${ident}" varname)
    set (flag -fsanitize=${_sanitize} -fno-sanitize-recover=all)

    cmake_push_check_state ()
        set (CMAKE_REQUIRED_FLAGS "${flag}")
        set (CMAKE_REQUIRED_LIBRARIES "${flag}")
        check_c_source_compiles ([[
            #include <stdio.h>

            int main (void) {
                puts ("Hello, world!");
                return 0;
            }
        ]] "${varname}")
    cmake_pop_check_state ()

    if (NOT "${${varname}}")
        message (SEND_ERROR "Requested sanitizer option '${flag}' is not supported by the compiler+linker")
    else ()
        message (STATUS "Enabling sanitizers: ${flag}")
        mongo_platform_compile_options ($<BUILD_INTERFACE:${flag}>)
        mongo_platform_link_options (${flag})
    endif ()
endif ()
