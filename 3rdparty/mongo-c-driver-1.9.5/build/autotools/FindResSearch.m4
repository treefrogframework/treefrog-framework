# Windows only.
AC_SUBST(MONGOC_HAVE_DNSAPI, 0)

AS_IF([test "x$enable_srv" = "xyes" -o "x$enable_srv" = "xauto"], [
  # Thread-safe DNS query function for _mongoc_client_get_srv.
  # Could be a macro, not a function, so check with AC_TRY_LINK.
  AC_MSG_CHECKING([for res_nsearch])
  save_LIBS="$LIBS"
  LIBS="$LIBS -lresolv"
  AC_TRY_LINK([
     #include <sys/types.h>
     #include <netinet/in.h>
     #include <arpa/nameser.h>
     #include <resolv.h>
  ],[
     int len;
     unsigned char reply[1024];
     res_state statep;
     len = res_nsearch(
        statep, "example.com", ns_c_in, ns_t_srv, reply, sizeof(reply));
  ],[
     AC_MSG_RESULT([yes])
     AC_SUBST(MONGOC_HAVE_RES_SEARCH, 0)
     AC_SUBST(MONGOC_HAVE_RES_NSEARCH, 1)
     AC_SUBST(RESOLV_LIBS, -lresolv)
     enable_srv=yes

     # We have res_nsearch. Call res_ndestroy (BSD/Mac) or res_nclose (Linux)?
     AC_MSG_CHECKING([for res_ndestroy])
     AC_TRY_LINK([
        #include <sys/types.h>
        #include <netinet/in.h>
        #include <arpa/nameser.h>
        #include <resolv.h>
     ],[
        res_state statep;
        res_ndestroy(statep);
     ], [
        AC_MSG_RESULT([yes])
        AC_SUBST(MONGOC_HAVE_RES_NDESTROY, 1)
        AC_SUBST(MONGOC_HAVE_RES_NCLOSE, 0)
     ], [
        AC_MSG_RESULT([no])
        AC_SUBST(MONGOC_HAVE_RES_NDESTROY, 0)

        AC_MSG_CHECKING([for res_nclose])
        AC_TRY_LINK([
           #include <sys/types.h>
           #include <netinet/in.h>
           #include <arpa/nameser.h>
           #include <resolv.h>
        ],[
           res_state statep;
           res_nclose(statep);
        ], [
           AC_MSG_RESULT([yes])
           AC_SUBST(MONGOC_HAVE_RES_NCLOSE, 1)
        ], [
           AC_MSG_RESULT([no])
           AC_SUBST(MONGOC_HAVE_RES_NCLOSE, 0)
        ])
     ])
  ],[
     AC_SUBST(MONGOC_HAVE_RES_NSEARCH, 0)
     AC_SUBST(MONGOC_HAVE_RES_NDESTROY, 0)
     AC_SUBST(MONGOC_HAVE_RES_NCLOSE, 0)

     AC_MSG_RESULT([no])
     AC_MSG_CHECKING([for res_search])

     # Thread-unsafe function.
     AC_TRY_LINK([
        #include <sys/types.h>
        #include <netinet/in.h>
        #include <arpa/nameser.h>
        #include <resolv.h>
     ],[
        int len;
        unsigned char reply[1024];
        len = res_search("example.com", ns_c_in, ns_t_srv, reply, sizeof(reply));
     ], [
        AC_MSG_RESULT([yes])
        AC_SUBST(MONGOC_HAVE_RES_SEARCH, 1)
        AC_SUBST(RESOLV_LIBS, -lresolv)
        enable_srv=yes
     ], [
        AC_MSG_RESULT([no])
        AC_SUBST(MONGOC_HAVE_RES_SEARCH, 0)
     ])
  ])

  LIBS="$save_LIBS"

], [
  # enable_srv = "no"

  AC_SUBST(MONGOC_HAVE_RES_NSEARCH, 0)
  AC_SUBST(MONGOC_HAVE_RES_NDESTROY, 0)
  AC_SUBST(MONGOC_HAVE_RES_NCLOSE, 0)
  AC_SUBST(MONGOC_HAVE_RES_SEARCH, 0)
])

AS_IF([test "x${RESOLV_LIBS}" = "x" -a "x$enable_srv" = "xyes"],
      [AC_MSG_ERROR([Cannot find libresolv. Try --disable_srv])])
