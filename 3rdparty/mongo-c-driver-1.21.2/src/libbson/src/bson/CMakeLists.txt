set (src_libbson_src_bson_DIST_hs
   bcon.h
   bson.h
   bson-atomic.h
   bson-clock.h
   bson-compat.h
   bson-context.h
   bson-decimal128.h
   bson-endian.h
   bson-error.h
   bson-iter.h
   bson-json.h
   bson-keys.h
   bson-macros.h
   bson-md5.h
   bson-memory.h
   bson-oid.h
   bson-reader.h
   bson-string.h
   bson-types.h
   bson-utf8.h
   bson-value.h
   bson-version-functions.h
   bson-writer.h
   bson-prelude.h
   bson-private.h
   bson-iso8601-private.h
   bson-context-private.h
   bson-timegm-private.h
   bson-json-private.h
   forwarding/bson.h
)
extra_dist_generated (
   bson-version.h
)

set (src_libbson_src_bson_DIST_cs
   bcon.c
   bson.c
   bson-atomic.c
   bson-clock.c
   bson-context.c
   bson-decimal128.c
   bson-error.c
   bson-iter.c
   bson-iso8601.c
   bson-json.c
   bson-keys.c
   bson-md5.c
   bson-memory.c
   bson-oid.c
   bson-reader.c
   bson-string.c
   bson-timegm.c
   bson-utf8.c
   bson-value.c
   bson-version-functions.c
   bson-writer.c
)

set_dist_list (src_libbson_src_bson_DIST
   CMakeLists.txt
   bson-config.h.in
   bson-version.h.in
   modules/module.modulemap.in
   ${src_libbson_src_bson_DIST_hs}
   ${src_libbson_src_bson_DIST_cs}
)
