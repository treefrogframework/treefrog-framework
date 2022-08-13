:man_page: bson_valgrind

Use Valgrind to Check For BSON Data Leaks
=========================================

A stack-allocated :symbol:`bson_t` contains a small internal buffer; it only heap-allocates additional storage if necessary, depending on its data size. Therefore if you forget to call :symbol:`bson_destroy` on a stack-allocated :symbol:`bson_t`, it might or might not cause a leak that can be detected by valgrind during testing.

To catch all potential BSON data leaks in your code, configure the BSON_MEMCHECK flag:

.. code-block:: none

   cmake -DCMAKE_C_FLAGS="-DBSON_MEMCHECK -g" .

With this flag set, every :symbol:`bson_t` mallocs at least one byte. Run your program's unittests with valgrind to verify all :symbol:`bson_t` structs are destroyed.

Set the environment variable ``MONGOC_TEST_VALGRIND`` to ``on`` to skip timing-dependent tests known to fail with valgrind.
