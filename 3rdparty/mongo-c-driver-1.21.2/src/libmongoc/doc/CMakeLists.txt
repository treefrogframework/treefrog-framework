include (SphinxBuild)

if (ENABLE_HTML_DOCS)
   sphinx_build_html (mongoc-html mongo-c-driver)
   set (src_libmongoc_doc_DIST_htmls ${doc_DIST_htmls})
endif ()

if (ENABLE_MAN_PAGES)
   sphinx_build_man (mongoc-man)
   set (src_libmongoc_doc_DIST_mans ${doc_DIST_mans})
endif ()

add_subdirectory (html)
add_subdirectory (includes)
add_subdirectory (man)
add_subdirectory (static)

file (GLOB src_libmongoc_doc_DIST_rsts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.rst)

extra_dist_generated (
   ${src_libmongoc_doc_DIST_htmls}
   ${src_libmongoc_doc_DIST_mans}
)

set_local_dist (src_libmongoc_doc_DIST_local
   CMakeLists.txt
   ${src_libmongoc_doc_DIST_rsts}
   conf.py
   libbson-objects.inv
)

set (src_libmongoc_doc_DIST
   ${src_libmongoc_doc_DIST_local}
   ${src_libmongoc_doc_html_DIST}
   ${src_libmongoc_doc_includes_DIST}
   ${src_libmongoc_doc_man_DIST}
   ${src_libmongoc_doc_mongoc_DIST}
   ${src_libmongoc_doc_mongoc-theme_DIST}
   ${src_libmongoc_doc_static_DIST}
   PARENT_SCOPE
)
