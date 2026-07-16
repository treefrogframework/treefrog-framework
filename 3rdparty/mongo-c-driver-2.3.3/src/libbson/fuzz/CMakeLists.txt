add_library(fuzz-properties INTERFACE)
target_link_libraries(fuzz-properties INTERFACE bson_static)
set_property(TARGET fuzz-properties APPEND PROPERTY INTERFACE_LINK_OPTIONS -fsanitize=fuzzer)

add_executable(fuzz-init-from-json EXCLUDE_FROM_ALL init-from-json.c)
target_link_libraries(fuzz-init-from-json PRIVATE fuzz-properties)

if(CMAKE_CXX_COMPILER)
    add_executable(fuzz-validate EXCLUDE_FROM_ALL validate.cpp)
    target_link_libraries(fuzz-validate PRIVATE fuzz-properties)
    add_executable(validate-repro EXCLUDE_FROM_ALL validate-repro.cpp)
    target_link_libraries(validate-repro PRIVATE bson_static)
endif()

add_executable(fuzz-bson-utf8-escape-for-json EXCLUDE_FROM_ALL utf8-escape-for-json.c)
target_link_libraries(fuzz-bson-utf8-escape-for-json PRIVATE fuzz-properties)
