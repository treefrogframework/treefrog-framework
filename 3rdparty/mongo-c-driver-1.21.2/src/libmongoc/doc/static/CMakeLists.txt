file (GLOB src_libmongoc_doc_static_DIST_pngs
   RELATIVE
   ${CMAKE_CURRENT_SOURCE_DIR}
   *.png
)

set_dist_list (src_libmongoc_doc_static_DIST
   CMakeLists.txt
   ${src_libmongoc_doc_static_DIST_pngs}
)
