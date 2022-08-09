add_subdirectory (cmake)

set_local_dist (src_libmongoc_build_DIST_local
   CMakeLists.txt
)

set (src_libmongoc_build_DIST
   ${src_libmongoc_build_DIST_local}
   ${src_libmongoc_build_cmake_DIST}
   PARENT_SCOPE
)
