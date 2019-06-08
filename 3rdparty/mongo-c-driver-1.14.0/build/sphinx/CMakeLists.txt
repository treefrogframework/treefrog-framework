add_subdirectory (mongoc-theme)

set_local_dist (build_sphinx_DIST_local
   CMakeLists.txt
   mongoc_common.py
   mongoc/__init__.py
   taglist.py
)

set (build_sphinx_DIST
   ${build_sphinx_DIST_local}
   ${build_sphinx_mongoc-theme_DIST}
   PARENT_SCOPE
)
