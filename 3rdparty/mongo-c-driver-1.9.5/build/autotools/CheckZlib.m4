# If --with-zlib=auto, determine if there is a system installed zlib
# greater than our required version.
found_zlib=no

AS_IF([test "x${with_zlib}" = xauto -o "x${with_zlib}" = xsystem], [
   PKG_CHECK_MODULES(ZLIB, [zlib], [
      found_zlib=yes
   ], [
      # If we didn't find zlib with pkgconfig, search manually. If that
      # fails and with-zlib=system, fail, or if with-zlib=auto, use
      # bundled.
      AC_CHECK_LIB([zlib], [compress2], [
         AC_CHECK_HEADER([zlib.h], [
            found_zlib=yes
            ZLIB_LIBS=-lz
         ])
      ])
   ])
], [
   AS_IF([test "x${with_zlib}" != xbundled -a "x${with_zlib}" != xno], [
      AC_MSG_ERROR([Invalid --with-zlib option: must be system, bundled, auto, or no.])
   ])
])

AS_IF([test "x${found_zlib}" = "xyes"], [
   with_zlib=system
], [
   # zlib not found
   AS_IF([test "x${with_zlib}" = xauto -o "x${with_zlib}" = xbundled], [
      with_zlib=bundled
   ], [
      AS_IF([test "x${with_zlib}" = xno], [], [
         # zlib not found, with-zlib=system
         AC_MSG_ERROR([Cannot find system installed zlib. try --with-zlib=bundled])
      ])
   ])
])


# If we are using the bundled zlib, recurse into its configure.
AS_IF([test "x${with_zlib}" = xbundled],[
   AC_MSG_CHECKING(whether to enable bundled zlib)
   AC_MSG_RESULT(yes)
   ZLIB_LIBS=
   ZLIB_CFLAGS="-Isrc/zlib-1.2.11"
])

if test "x${with_zlib}" != "xno"; then
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_ZLIB, 1)
else
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_ZLIB, 0)
fi
AC_SUBST(ZLIB_LIBS)
AC_SUBST(ZLIB_CFLAGS)

