set (build_cmake_make_dist_MODULES
   MakeDist.cmake
   MakeDistCheck.cmake
   MakeDistFiles.cmake
)

set_dist_list (build_cmake_make_dist_DIST
   CMakeLists.txt
   COPYING-CMAKE-SCRIPTS.txt
   ${build_cmake_make_dist_MODULES}
)
