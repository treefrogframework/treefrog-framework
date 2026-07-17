
set (MONGOC_ENABLE_DEBUG_ASSERTIONS 0)

if (ENABLE_DEBUG_ASSERTIONS)
    set (MONGOC_ENABLE_DEBUG_ASSERTIONS 1)
endif ()

configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/src/common-config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/src/common-config.h"
)

add_library(mongo-mlib INTERFACE)
add_library(mongo::mlib ALIAS mongo-mlib)
target_include_directories(mongo-mlib INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)
set_property(TARGET mongo-mlib PROPERTY EXPORT_NAME mongo::mlib)

if(CMAKE_CXX_COMPILER)
    add_executable(mlib-ckdint-test src/mlib/ckdint.test.cpp)
    set_target_properties(mlib-ckdint-test PROPERTIES
        COMPILE_FEATURES cxx_std_11
        LINK_LIBRARIES mongo::mlib
        # Enable -fPIC, required for some build configurations
        POSITION_INDEPENDENT_CODE TRUE
        )
    add_test(NAME mongoc/mlib/ckdint COMMAND mlib-ckdint-test)
    set_property(TEST mongoc/mlib/ckdint PROPERTY SKIP_REGULAR_EXPRESSION "@@ctest-skipped@@")
endif()
