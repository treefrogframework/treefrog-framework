cmake_minimum_required (VERSION 3.15...4.0)
project(ImportTestProject)

include(CTest)

add_compile_definitions("EXPECT_BSON_VERSION=\"${EXPECT_BSON_VERSION}\"")
add_compile_definitions("EXPECT_MONGOC_VERSION=\"${EXPECT_MONGOC_VERSION}\"")

if(FIND_BSON)
    # Call find_package() twice to check that we can be imported twice
    find_package(bson ${FIND_BSON_ARGS})
    find_package(bson ${FIND_BSON_ARGS})
endif()

if(FIND_MONGOC)
    find_package(mongoc ${FIND_MONGOC_ARGS})
    find_package(mongoc ${FIND_MONGOC_ARGS})
endif()

if(bson_FOUND AND EXPECT_FIND_BSON_FAILS)
    message(FATAL_ERROR "We expected find_package(bson) to fail, but it succeeded [bson_DIR=${bson_DIR}]")
endif()

if(mongoc_FOUND AND EXPECT_FIND_MONGOC_FAILS)
    message(FATAL_ERROR "We expected find_package(mongoc) to fail, but it succeeded [mongoc_DIR=${mongoc_DIR}]")
endif()

if(EXPECT_BSON_VERSION)
    add_executable(use-bson use-bson.c)
    target_link_libraries(use-bson PRIVATE bson::bson)
    add_test(use-bson-test use-bson)
endif()

if(EXPECT_MONGOC_VERSION)
    add_executable(use-mongoc use-mongoc.c)
    target_link_libraries(use-mongoc PRIVATE mongoc::mongoc)
    add_test(use-mongoc-test use-mongoc)
endif()
