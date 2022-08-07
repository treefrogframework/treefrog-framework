# function (sphinx_build_html)
#
# Parameters:
#
# target_name - name of the generated custom target (e.g., bson-html)
# doc_dir - name of the destination directory for HTML documentation; is used
# to construct a target path: ${CMAKE_INSTALL_PREFIX}/share/doc/${doc_dir}/html
#
# Description:
#
# Process the list of .rst files in the current source directory and:
# - build up a list of target/output .html files
# - create the custom Sphinx command that produces the HTML output
# - add install rules for:
#   + source static content (e.g., images)
#   + each output file
#   + additional Sphinx-generated content
# - create the custom target that depends on the output files and calls sphinx
# - set doc_DIST_rsts and doc_DIST_htmls in the parent scope with the lists of
#   source .rst and output .html files, respectively, for making distributions
#
function (sphinx_build_html target_name doc_dir)
   include (ProcessorCount)
   ProcessorCount (NPROCS)

   set (SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

   file (GLOB doc_rsts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.rst)

   foreach (rst IN LISTS doc_rsts)
      # Every .rst builds a corresponding .html
      string (REGEX REPLACE "^([^.]+)\.rst$" "html/\\1.html" html ${rst})
      list (APPEND doc_htmls ${html})
   endforeach ()

   # Set PYTHONDONTWRITEBYTECODE to prevent .pyc clutter in the source directory
   add_custom_command (OUTPUT ${doc_htmls}
      ${SPHINX_HTML_DIR}/.nojekyll ${SPHINX_HTML_DIR}/objects.inv
      COMMAND
      ${CMAKE_COMMAND} -E env
      "PYTHONDONTWRITEBYTECODE=1"
      ${SPHINX_EXECUTABLE}
         -qEW -b html
         -c "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}"
         "${SPHINX_HTML_DIR}"
      COMMAND
      rm -rf "${SPHINX_HTML_DIR}/.doctrees" "${SPHINX_HTML_DIR}/.buildinfo"
      DEPENDS
      ${doc_rsts}
      COMMENT
      "Building HTML documentation with Sphinx"
   )

   foreach (html IN LISTS doc_htmls)
      install (FILES
         ${CMAKE_CURRENT_BINARY_DIR}/${html}
         DESTINATION
         ${CMAKE_INSTALL_DOCDIR}/${doc_dir}/html
      )
   endforeach ()

   # Ensure additional Sphinx-generated content gets installed
   install (FILES
      ${SPHINX_HTML_DIR}/search.html
      ${SPHINX_HTML_DIR}/objects.inv
      ${SPHINX_HTML_DIR}/searchindex.js
      DESTINATION
      ${CMAKE_INSTALL_DOCDIR}/${doc_dir}/html
   )
   install (DIRECTORY
      ${SPHINX_HTML_DIR}/_static
      DESTINATION
      ${CMAKE_INSTALL_DOCDIR}/${doc_dir}/html
   )
   if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/static)
      install (DIRECTORY
         ${SPHINX_HTML_DIR}/_images
         DESTINATION
         ${CMAKE_INSTALL_DOCDIR}/${doc_dir}/html
      )
   endif ()

   add_custom_target (${target_name} DEPENDS ${doc_htmls})

   # Pass lists back up for building distributions
   set (doc_DIST_rsts ${doc_rsts} PARENT_SCOPE)
   set (doc_DIST_htmls ${doc_htmls} PARENT_SCOPE)
endfunction ()

# function (sphinx_build_man)
#
# Parameters:
#
# target_name - name of the generated custom target (e.g., mongoc-man)
#
# Description:
#
# Process the list of .rst files in the current source directory and:
# - build up a list of target/output .3 files (i.e., man pages)
# - create the custom Sphinx command that produces the man page output
# - add install rules for each output file
# - create the custom target that depends on the output files and calls sphinx
# - set doc_DIST_rsts and doc_DIST_mans in the parent scope with the lists of
#   source .rst and output .html files, respectively, for making distributions
#
function (sphinx_build_man target_name)
   include (ProcessorCount)
   ProcessorCount (NPROCS)

   set (SPHINX_MAN_DIR "${CMAKE_CURRENT_BINARY_DIR}/man")

   file (GLOB doc_rsts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.rst)

   foreach (rst IN LISTS doc_rsts)
      # Only those with the :man_page: tag at the beginning build man pages
      file (READ ${rst} rst_head LIMIT 256)
      string (FIND "${rst_head}" ":man_page: " man_tag_pos)
      # GREATER_EQUAL not in CMake until 3.7.
      if (NOT man_tag_pos LESS "0")
         string (REGEX REPLACE
            "^.*:man_page: +([a-z0-9_]+).*$" "man\/\\1.3"
            man
            ${rst_head}
         )
         list (APPEND doc_mans ${man})
      endif ()
   endforeach ()

   # Set PYTHONDONTWRITEBYTECODE to prevent .pyc clutter in the source directory
   add_custom_command (OUTPUT ${doc_mans}
      COMMAND
      ${CMAKE_COMMAND} -E env
      "PYTHONDONTWRITEBYTECODE=1"
      ${SPHINX_EXECUTABLE}
         -qEW -b man
         -c "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}"
         "${SPHINX_MAN_DIR}"
      COMMAND
      rm -rf "${SPHINX_MAN_DIR}/.doctrees" "${SPHINX_MAN_DIR}/.buildinfo"
      DEPENDS
      ${doc_rsts}
      COMMENT
      "Building manual pages with Sphinx"
   )

   foreach (man IN LISTS doc_mans)
      install (FILES
         ${CMAKE_CURRENT_BINARY_DIR}/${man}
         DESTINATION
         ${CMAKE_INSTALL_MANDIR}/man3
      )
   endforeach ()

   add_custom_target (${target_name} DEPENDS ${doc_mans})

   # Pass lists back up for building distributions
   set (doc_DIST_rsts ${doc_rsts} PARENT_SCOPE)
   set (doc_DIST_mans ${doc_mans} PARENT_SCOPE)
endfunction ()

