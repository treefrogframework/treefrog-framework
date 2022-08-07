file (GLOB_RECURSE src_libbson_examples_DIST_cs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)
file (GLOB_RECURSE src_libbson_examples_DIST_shs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.sh)

set_dist_list (src_libbson_examples_DIST
   CMakeLists.txt
   cmake/find_package/CMakeLists.txt
   cmake/find_package_static/CMakeLists.txt
   ${src_libbson_examples_DIST_cs}
   ${src_libbson_examples_DIST_shs}
)
