:man_page: bson_md5_init

bson_md5_init()
===============

Deprecated
----------
All MD5 APIs are deprecated in libbson.

Synopsis
--------

.. code-block:: c

  void
  bson_md5_init (bson_md5_t *pms) BSON_GNUC_DEPRECATED;

Parameters
----------

* ``pms``: A :symbol:`bson_md5_t`.

Description
-----------

Initialize a new instance of the MD5 algorithm.

This function is deprecated and should not be used in new code.
