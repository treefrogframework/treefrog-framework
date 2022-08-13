add_subdirectory (readable)

set_local_dist (build_sphinx_DIST_local
   CMakeLists.txt
   mongoc_common.py
   mongoc/__init__.py
   customindexlink.html
)

set (build_sphinx_DIST
   ${build_sphinx_DIST_local}
   ${build_sphinx_readable_DIST}
   PARENT_SCOPE
)
