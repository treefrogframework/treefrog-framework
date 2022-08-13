set (src_libbson_build_cmake_MODULES
   BSONPackage.cmake
)

set (src_libbson_build_cmake_PKGCONFIG
   libbson-1.0-config-version.cmake.in
   libbson-1.0-config.cmake.in
   libbson-static-1.0-config-version.cmake.in
   libbson-static-1.0-config.cmake.in
)

set_local_dist (src_libbson_build_cmake_DIST_local
   CMakeLists.txt
   ${src_libbson_build_cmake_MODULES}
   ${src_libbson_build_cmake_PKGCONFIG}
)

set (src_libbson_build_cmake_DIST
   ${src_libbson_build_cmake_DIST_local}
   PARENT_SCOPE
)
