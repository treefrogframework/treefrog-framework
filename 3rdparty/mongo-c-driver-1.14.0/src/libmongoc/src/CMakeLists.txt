add_subdirectory (mongoc)

set_local_dist (src_libmongoc_src_DIST_local
   CMakeLists.txt
   libmongoc-1.0.pc.in
   libmongoc-ssl-1.0.pc.in
   libmongoc-static-1.0.pc.in
)

set (src_libmongoc_src_DIST
   ${src_libmongoc_src_DIST_local}
   ${src_libmongoc_src_mongoc_DIST}
   PARENT_SCOPE
)

