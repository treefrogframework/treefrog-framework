:man_page: bson_md5_t

bson_md5_t
==========

BSON MD5 Abstraction

Deprecated
----------
All MD5 APIs are deprecated in libbson.

Synopsis
--------

.. code-block:: c

  typedef struct {
     uint32_t count[2]; /* message length in bits, lsw first */
     uint32_t abcd[4];  /* digest buffer */
     uint8_t buf[64];   /* accumulate block */
  } bson_md5_t;

Description
-----------

:symbol:`bson_md5_t` encapsulates an implementation of the MD5 algorithm.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_md5_append
    bson_md5_finish
    bson_md5_init

