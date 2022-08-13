:man_page: mongoc_cursor_is_alive

mongoc_cursor_is_alive()
========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_is_alive (const mongoc_cursor_t *cursor)
     BSON_GNUC_DEPRECATED_FOR (mongoc_cursor_more);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Deprecated
----------

This function is superseded by :symbol:`mongoc_cursor_more()`, which has equivalent behavior.

Returns
-------

See :symbol:`mongoc_cursor_more()`.
