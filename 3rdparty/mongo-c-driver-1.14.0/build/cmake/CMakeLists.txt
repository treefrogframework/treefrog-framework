add_subdirectory (make_dist)

set (build_cmake_MODULES
   CheckSchedGetCPU.cmake
   FindResSearch.cmake
   FindSASL2.cmake
   FindSnappy.cmake
   FindSphinx.cmake
   LoadVersion.cmake
   MaintainerFlags.cmake
   MongoCPackage.cmake
   ParseVersion.cmake
   SphinxBuild.cmake
)

set_local_dist (build_cmake_DIST_local
   CMakeLists.txt
   ${build_cmake_MODULES}
)

set (build_cmake_DIST
   ${build_cmake_DIST_local}
   ${build_cmake_make_dist_DIST}
   PARENT_SCOPE
)
