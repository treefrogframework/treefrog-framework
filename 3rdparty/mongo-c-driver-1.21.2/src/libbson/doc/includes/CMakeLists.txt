file (GLOB src_libbson_doc_includes_DIST_txts
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   seealso/*.txt
)

set_dist_list (src_libbson_doc_includes_DIST
   CMakeLists.txt
   ${src_libbson_doc_includes_DIST_txts}
)

