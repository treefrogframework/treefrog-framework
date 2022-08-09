add_subdirectory (bson)
add_subdirectory (jsonsl)

set_local_dist (src_libbson_src_DIST_local
   CMakeLists.txt
   bson-config.cmake
   libbson-1.0.pc.in
   libbson-static-1.0.pc.in
)

set (src_libbson_src_DIST
   ${src_libbson_src_DIST_local}
   ${src_libbson_src_bson_DIST}
   ${src_libbson_src_jsonsl_DIST}
   PARENT_SCOPE
)
