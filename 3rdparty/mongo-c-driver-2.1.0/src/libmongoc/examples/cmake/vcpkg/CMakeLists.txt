cmake_minimum_required (VERSION 3.15...4.0)
project(vcpkg-example-project)

find_package(mongoc CONFIG REQUIRED)
message(STATUS "Found libmongoc: ${mongoc_DIR}")

enable_testing()

add_executable(my-app app.c)
target_link_libraries(my-app PRIVATE mongoc::static)
add_test(my-app my-app)
set_property(
    TEST my-app
    PROPERTY PASS_REGULAR_EXPRESSION
    "^Linked with libmongoc [1-9]+\\.[0-9]+\\.[0-9]+\r?\n$"
)
