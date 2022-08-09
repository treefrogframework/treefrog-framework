set (src_libbson_src_jsonsl_DIST_hs
   jsonsl.h
)

set (src_libbson_src_jsonsl_DIST_cs
   jsonsl.c
)

set_dist_list (src_libbson_src_jsonsl_DIST
   CMakeLists.txt
   ${src_libbson_src_jsonsl_DIST_hs}
   ${src_libbson_src_jsonsl_DIST_cs}
)
