include (CheckSymbolExists)

if (ENABLE_SRV STREQUAL ON OR ENABLE_SRV STREQUAL AUTO)
   if (WIN32)
      set (RESOLV_LIBRARIES Dnsapi)
      set (MONGOC_HAVE_DNSAPI 1)
      set (MONGOC_HAVE_RES_NSEARCH 0)
      set (MONGOC_HAVE_RES_NDESTROY 0)
      set (MONGOC_HAVE_RES_NCLOSE 0)
      set (MONGOC_HAVE_RES_SEARCH 0)
   else ()
      set (MONGOC_HAVE_DNSAPI 0)
      # Thread-safe DNS query function for _mongoc_client_get_srv.
      # Could be a macro, not a function, so use check_symbol_exists.
      check_symbol_exists (res_nsearch resolv.h MONGOC_HAVE_RES_NSEARCH)
      if (MONGOC_HAVE_RES_NSEARCH)
         set (RESOLV_LIBRARIES resolv)
         set (MONGOC_HAVE_RES_SEARCH 0)

         # We have res_nsearch. Call res_ndestroy (BSD/Mac) or res_nclose (Linux)?
         check_symbol_exists (res_ndestroy resolv.h MONGOC_HAVE_RES_NDESTROY)
         if (MONGOC_HAVE_RES_NDESTROY)
            set (MONGOC_HAVE_RES_NCLOSE 0)
         else ()
            set (MONGOC_HAVE_RES_NDESTROY 0)
            check_symbol_exists (res_nclose resolv.h MONGOC_HAVE_RES_NCLOSE)
            if (NOT MONGOC_HAVE_RES_NCLOSE)
               set (MONGOC_HAVE_RES_NCLOSE 0)
            endif ()
         endif ()
      else ()
         set (MONGOC_HAVE_RES_NSEARCH 0)
         set (MONGOC_HAVE_RES_NDESTROY 0)
         set (MONGOC_HAVE_RES_NCLOSE 0)

         # Thread-unsafe function.
         check_symbol_exists (res_search resolv.h MONGOC_HAVE_RES_SEARCH)
         if (MONGOC_HAVE_RES_SEARCH)
            set (RESOLV_LIBRARIES resolv)
         else ()
            set (MONGOC_HAVE_RES_SEARCH 0)
         endif ()
      endif ()
   endif ()
else ()
   # ENABLE_SRV disabled, set default values for defines.
   set (MONGOC_HAVE_DNSAPI 0)
   set (MONGOC_HAVE_RES_NSEARCH 0)
   set (MONGOC_HAVE_RES_NDESTROY 0)
   set (MONGOC_HAVE_RES_NCLOSE 0)
   set (MONGOC_HAVE_RES_SEARCH 0)  
endif ()

if (ENABLE_SRV STREQUAL ON AND NOT RESOLV_LIBRARIES)
   message (
      FATAL_ERROR
      "Cannot find libresolv or dnsapi. Try setting ENABLE_SRV=OFF")
endif ()
