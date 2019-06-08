add_subdirectory (cmake)

set_local_dist (src_libbson_build_DIST_local
   CMakeLists.txt
   maintainer-flags.txt
)

set (src_libbson_build_DIST
   ${src_libbson_build_DIST_local}
   ${src_libbson_build_cmake_DIST}
   PARENT_SCOPE
)
