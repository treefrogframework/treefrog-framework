
set (MONGOC_ENABLE_DEBUG_ASSERTIONS 0)

if (ENABLE_DEBUG_ASSERTIONS)
    set (MONGOC_ENABLE_DEBUG_ASSERTIONS 1)
endif ()

include (MongoC-Warnings)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/common/common-config.h.in"
        "${PROJECT_BINARY_DIR}/src/common/common-config.h"
)

set (src_common_DIST_noinst_hs
   common-b64-private.h
   common-md5-private.h
   common-prelude.h
   common-thread-private.h
   common-config.h.in
   common-macros-private.h
)

set (src_common_DIST_cs
   common-b64.c
   common-md5.c
   common-thread.c
)

set_dist_list (src_common_DIST
   CMakeLists.txt
   ${src_common_DIST_cs}
   ${src_common_DIST_noinst_hs}
)
