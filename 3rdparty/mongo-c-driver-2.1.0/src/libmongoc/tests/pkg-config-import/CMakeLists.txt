cmake_minimum_required (VERSION 3.15...4.0)
project(pkg-config-import-test LANGUAGES C)

# This is a test case that tries to build against bson/mongoc using pkg-config.
# Even though this is a CMake project, we import by talking to `pkg-config`

include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(thing REQUIRED IMPORTED_TARGET "${PKG_CONFIG_SEARCH}")

add_compile_definitions(
    "EXPECT_BSON_VERSION=\"${EXPECT_BSON_VERSION}\""
    "EXPECT_MONGOC_VERSION=\"${EXPECT_MONGOC_VERSION}\""
    EXPECT_BSON_STATIC=$<BOOL:${EXPECT_BSON_STATIC}>
    EXPECT_MONGOC_STATIC=$<BOOL:${EXPECT_MONGOC_STATIC}>
    )

add_executable(my-app app.c)
target_link_libraries(my-app PRIVATE PkgConfig::thing)

if(EXPECT_BSON_VERSION)
    add_executable(use-bson use-bson.c)
    target_link_libraries(use-bson PRIVATE PkgConfig::thing)
    add_test(use-bson use-bson)
endif()

if(EXPECT_MONGOC_VERSION)
    add_executable(use-mongoc use-mongoc.c)
    target_link_libraries(use-mongoc PRIVATE PkgConfig::thing)
    add_test(use-mongoc use-mongoc)
endif()
