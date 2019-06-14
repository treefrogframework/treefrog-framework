include (CheckCSourceCompiles)

check_c_source_compiles ("
   #include <stdint.h>

   int
   main ()
   {
      int32_t v = 1;
      return __sync_add_and_fetch_4 (&v, (int32_t) 10);
   }"
   HAVE_ATOMIC_32_ADD_AND_FETCH
)

if (HAVE_ATOMIC_32_ADD_AND_FETCH)
   set (BSON_HAVE_ATOMIC_32_ADD_AND_FETCH 1)
else ()
   set (BSON_HAVE_ATOMIC_32_ADD_AND_FETCH 0)
endif ()

check_c_source_compiles ("
   #include <stdint.h>

   int
   main ()
   {
      int64_t v = 1;
      return __sync_add_and_fetch_8 (&v, (int64_t) 10);
   }"
   HAVE_ATOMIC_64_ADD_AND_FETCH
)

if (HAVE_ATOMIC_64_ADD_AND_FETCH)
   set (BSON_HAVE_ATOMIC_64_ADD_AND_FETCH 1)
else ()
   set (BSON_HAVE_ATOMIC_64_ADD_AND_FETCH 0)
endif ()
