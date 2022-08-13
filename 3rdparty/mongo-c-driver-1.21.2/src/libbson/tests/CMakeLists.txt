file (GLOB_RECURSE src_libbson_tests_DIST_cs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)
file (GLOB_RECURSE src_libbson_tests_DIST_hs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)
file (GLOB_RECURSE src_libbson_tests_DIST_bsons RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.bson)
file (GLOB_RECURSE src_libbson_tests_DIST_jsons RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.json)

set_dist_list (src_libbson_tests_DIST
   CMakeLists.txt
   ${src_libbson_tests_DIST_cs}
   ${src_libbson_tests_DIST_hs}
   ${src_libbson_tests_DIST_bsons}
   ${src_libbson_tests_DIST_jsons}
)
