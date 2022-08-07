include (SphinxBuild)

if (ENABLE_HTML_DOCS)
   sphinx_build_html (bson-html libbson)
   set (src_libbson_doc_DIST_htmls ${doc_DIST_htmls})
endif ()

if (ENABLE_MAN_PAGES)
   sphinx_build_man (bson-man)
   set (src_libbson_doc_DIST_mans ${doc_DIST_mans})
endif ()

add_subdirectory (html)
add_subdirectory (includes)
add_subdirectory (man)

file (GLOB src_libbson_doc_DIST_rsts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.rst)

extra_dist_generated (
   ${src_libbson_doc_DIST_htmls}
   ${src_libbson_doc_DIST_mans}
)

set_local_dist (src_libbson_doc_DIST_local
   CMakeLists.txt
   ${src_libbson_doc_DIST_rsts}
   conf.py
)

set (src_libbson_doc_DIST
   ${src_libbson_doc_DIST_local}
   ${src_libbson_doc_html_DIST}
   ${src_libbson_doc_includes_DIST}
   ${src_libbson_doc_man_DIST}
   ${src_libbson_doc_mongoc_DIST}
   ${src_libbson_doc_mongoc-theme_DIST}
   PARENT_SCOPE
)
