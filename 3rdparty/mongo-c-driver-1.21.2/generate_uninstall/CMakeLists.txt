# Fabricate our own copy of the install manifest, since the installation has not
# generated the final version yet at this point

set (UNINSTALL_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if (WIN32)
   string (REPLACE "/" "\\\\" CMAKE_INSTALL_PREFIX_WIN32
      "${CMAKE_INSTALL_PREFIX}"
   )
   install (CODE "
      string(REPLACE \";\" \"\\n\" MONGOC_INSTALL_MANIFEST_CONTENT
         \"\${CMAKE_INSTALL_MANIFEST_FILES}\")
      string(REPLACE \"/\" \"\\\\\" MONGOC_INSTALL_MANIFEST_CONTENT_WIN32
         \"\${MONGOC_INSTALL_MANIFEST_CONTENT}\")
      file(WRITE \"mongoc_install_manifest.txt\"
         \"\${MONGOC_INSTALL_MANIFEST_CONTENT_WIN32}\")
      execute_process (
         COMMAND
            ${CMAKE_COMMAND} -E env
            ENABLE_MONGOC=${ENABLE_MONGOC}
            cmd.exe /c
            \"${PROJECT_SOURCE_DIR}/build/generate-uninstall.cmd\"
            mongoc_install_manifest.txt
            ${CMAKE_INSTALL_PREFIX_WIN32}
         OUTPUT_FILE
            \"${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}\"
      )
   ")
   install (FILES "${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}" DESTINATION "${UNINSTALL_PROG_DIR}" PERMISSIONS ${UNINSTALL_PERMISSIONS})

   add_custom_target (uninstall
      COMMAND call "${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}"
   )
else ()
   install (CODE "
      string(REPLACE \";\" \"\\n\" MONGOC_INSTALL_MANIFEST_CONTENT
         \"\${CMAKE_INSTALL_MANIFEST_FILES}\")
      file(WRITE \"mongoc_install_manifest.txt\"
         \"\${MONGOC_INSTALL_MANIFEST_CONTENT}\")
      execute_process (
         COMMAND
            ${CMAKE_COMMAND} -E env
            ENABLE_MONGOC=${ENABLE_MONGOC}
            \"${PROJECT_SOURCE_DIR}/build/generate-uninstall.sh\"
            mongoc_install_manifest.txt
            ${CMAKE_INSTALL_PREFIX}
         OUTPUT_FILE
            \"${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}\"
      )
   ")
   install (FILES "${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}" DESTINATION "${UNINSTALL_PROG_DIR}" PERMISSIONS ${UNINSTALL_PERMISSIONS})

   add_custom_target (uninstall
      COMMAND sh "${CMAKE_CURRENT_BINARY_DIR}/${UNINSTALL_PROG}"
   )
endif ()
