set (src_libmongoc_build_cmake_PKGCONFIG
   libmongoc-1.0-config-version.cmake.in
   libmongoc-1.0-config.cmake.in
   libmongoc-static-1.0-config-version.cmake.in
   libmongoc-static-1.0-config.cmake.in
)

set_local_dist (src_libmongoc_build_cmake_DIST_local
   CMakeLists.txt
   ${src_libmongoc_build_cmake_PKGCONFIG}
)

set (src_libmongoc_build_cmake_DIST
   ${src_libmongoc_build_cmake_DIST_local}
   PARENT_SCOPE
)
