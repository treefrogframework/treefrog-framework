include(CMakeFindDependencyMacro)
find_dependency(bson-1.0 @MONGOC_MAJOR_VERSION@.@MONGOC_MINOR_VERSION@.@MONGOC_MICRO_VERSION@)
include("${CMAKE_CURRENT_LIST_DIR}/mongoc-targets.cmake")
