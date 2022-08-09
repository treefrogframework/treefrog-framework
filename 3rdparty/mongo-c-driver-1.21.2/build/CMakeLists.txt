add_subdirectory (cmake)
add_subdirectory (sphinx)

set_local_dist (build_DIST_local
   CMakeLists.txt
   maintainer-flags.txt
   generate-uninstall.cmd
   generate-uninstall.sh
)

set (build_DIST
   ${build_DIST_local}
   ${build_cmake_DIST}
   ${build_sphinx_DIST}
   PARENT_SCOPE
)
