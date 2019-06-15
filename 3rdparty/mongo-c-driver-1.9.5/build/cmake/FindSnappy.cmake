include(CheckSymbolExists)

if (NOT (ENABLE_SNAPPY STREQUAL SYSTEM
   OR ENABLE_SNAPPY STREQUAL AUTO
   OR ENABLE_SNAPPY STREQUAL OFF))
   message (FATAL_ERROR
      "ENABLE_SNAPPY option must be SYSTEM, AUTO, or OFF")
endif()


if (ENABLE_SNAPPY STREQUAL OFF)
   set (SNAPPY_INCLUDE_DIRS)
   set (SNAPPY_LIBS)
   set (MONGOC_ENABLE_COMPRESSION_SNAPPY 0)
else ()
   message (STATUS "Searching for compression library header snappy-c.h")
   find_path (
      SNAPPY_INCLUDE_DIRS NAMES snappy-c.h
      PATHS /include /usr/include /usr/local/include /usr/share/include /opt/include c:/snappy/include
      DOC "Searching for snappy-c.h")

   if (NOT SNAPPY_INCLUDE_DIRS)
      if (ENABLE_SNAPPY STREQUAL SYSTEM)
         message (FATAL_ERROR "  Not found (specify -DCMAKE_INCLUDE_PATH=C:/path/to/snappy/include for Snappy compression)")
      else ()
         message (STATUS "  Not found (specify -DCMAKE_INCLUDE_PATH=C:/path/to/snappy/include for Snappy compression)")
      endif ()
   else ()
      message (STATUS "  Found in ${SNAPPY_INCLUDE_DIRS}")
      message (STATUS "Searching for libsnappy")
      find_library (
         SNAPPY_LIBS NAMES snappy
         PATHS /usr/lib /lib /usr/local/lib /usr/share/lib /opt/lib /opt/share/lib /var/lib c:/snappy/lib
         DOC "Searching for libsnappy")

      if (SNAPPY_LIBS)
         message (STATUS "  Found ${SNAPPY_LIBS}")
      else ()
         if (ENABLE_SNAPPY STREQUAL SYSTEM)
            message (FATAL_ERROR "  Not found (specify -DCMAKE_LIBRARY_PATH=C:/path/to/snappy/lib for Snappy compression)")
         else ()
            message (STATUS "  Not found (specify -DCMAKE_LIBRARY_PATH=C:/path/to/snappy/lib for Snappy compression)")
         endif ()
      endif ()
   endif ()

   if (SNAPPY_INCLUDE_DIRS AND SNAPPY_LIBS)
      set (MONGOC_ENABLE_COMPRESSION_SNAPPY 1)
   else ()
      set (MONGOC_ENABLE_COMPRESSION_SNAPPY 0)
   endif ()
endif ()
