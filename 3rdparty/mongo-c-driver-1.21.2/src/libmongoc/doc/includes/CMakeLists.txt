file (GLOB src_libmongoc_doc_includes_DIST_txts
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.txt
   seealso/*.txt
)

set_dist_list (src_libmongoc_doc_includes_DIST
   CMakeLists.txt
   ${src_libmongoc_doc_includes_DIST_txts}
)
