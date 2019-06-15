# If --with-snappy=auto, determine if there is a system installed snappy
# greater than our required version.
found_snappy=no

AS_IF([test "x${with_snappy}" = xauto -o "x${with_snappy}" = xsystem], [
   PKG_CHECK_MODULES(SNAPPY, [snappy], [
      found_snappy=yes
   ], [
      # If we didn't find snappy with pkgconfig, search manually. If that
      # fails and with-snappy=system, fail.
      AC_CHECK_LIB([snappy], [snappy_uncompress], [
         AC_CHECK_HEADER([snappy-c.h], [
            found_snappy=yes
            SNAPPY_LIBS=-lsnappy
         ])
      ])
   ])
])

AS_IF([test "x${found_snappy}" = xyes], [
   with_snappy=system
], [
   # snappy not found
   AS_IF([test "x${with_snappy}" = xsystem], [
      AC_MSG_ERROR([Cannot find system installed snappy. try --with-snappy=no])
   ])
   with_snappy=no
])

if test "x${with_snappy}" != "xno"; then
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_SNAPPY, 1)
else
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_SNAPPY, 0)
fi
AC_SUBST(SNAPPY_LIBS)

