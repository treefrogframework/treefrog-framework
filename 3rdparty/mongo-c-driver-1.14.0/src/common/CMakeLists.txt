set (src_common_DIST_noinst_hs
   common-b64-private.h
   common-md5-private.h
   common-prelude.h
   common-thread-private.h
)

set (src_common_DIST_cs
   common-b64.c
   common-md5.c
)

set_dist_list (src_common_DIST
   CMakeLists.txt
   ${src_common_DIST_cs}
   ${src_common_DIST_noinst_hs}
)
