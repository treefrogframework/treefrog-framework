file (GLOB_RECURSE src_libmongoc_tests_DIST_cs
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.c
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_hs
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.h
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_zeros
   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   *.0
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_pems
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.pem
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_dats
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.dat
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_txts
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.txt
)
file (GLOB_RECURSE src_libmongoc_tests_DIST_jsons
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.json
)

set_dist_list (src_libmongoc_tests_DIST
   CMakeLists.txt
   ${src_libmongoc_tests_DIST_cs}
   ${src_libmongoc_tests_DIST_hs}
   ${src_libmongoc_tests_DIST_zeros}
   ${src_libmongoc_tests_DIST_pems}
   ${src_libmongoc_tests_DIST_dats}
   ${src_libmongoc_tests_DIST_txts}
   ${src_libmongoc_tests_DIST_jsons}
)
