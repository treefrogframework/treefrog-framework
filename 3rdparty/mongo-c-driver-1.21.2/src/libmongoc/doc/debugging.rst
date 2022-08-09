:man_page: mongoc_debugging

Aids for Debugging
==================

GDB
---

This repository contains a ``.gdbinit`` file that contains helper functions to
aid debugging of data structures. GDB will load this file
`automatically`_ if you have added the directory which contains the `.gdbinit` file to GDB's
`auto-load safe-path`_, *and* you start GDB from the directory which holds the `.gdbinit` file.

You can see the safe-path with ``show auto-load safe-path`` on a GDB prompt. You
can configure it by setting it in ``~/.gdbinit`` with::

  add-auto-load-safe-path /path/to/mongo-c-driver

If you haven't added the path to your auto-load safe-path, or start GDB in
another directory, load the file with::

  source path/to/mongo-c-driver/.gdbinit

The ``.gdbinit`` file defines the ``printbson`` function, which shows the contents of a ``bson_t *`` variable.
If you have a local ``bson_t``, then you must prefix the variable with a `&`.

An example GDB session looks like::

  (gdb) printbson bson
  ALLOC [0x555556cd7310 + 0] (len=475)
  {
      'bool' : true,
      'int32' : NumberInt("42"),
      'int64' : NumberLong("3000000042"),
      'string' : "Stŕìñg",
      'objectId' : ObjectID("5A1442F3122D331C3C6757E1"),
      'utcDateTime' : UTCDateTime(1511277299031),
      'arrayOfInts' : [
          '0' : NumberInt("1"),
          '1' : NumberInt("2")
      ],
      'embeddedDocument' : {
          'arrayOfStrings' : [
              '0' : "one",
              '1' : "two"
          ],
          'double' : 2.718280,
          'notherDoc' : {
              'true' : NumberInt("1"),
              'false' : false
          }
      },
      'binary' : Binary("02", "3031343532333637"),
      'regex' : Regex("@[a-z]+@", "im"),
      'null' : null,
      'js' : JavaScript("print foo"),
      'jsws' : JavaScript("print foo") with scope: {
          'f' : NumberInt("42"),
          'a' : [
              '0' : 3.141593,
              '1' : 2.718282
          ]
      },
      'timestamp' : Timestamp(4294967295, 4294967295),
      'double' : 3.141593
  }

.. _automatically: https://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading.html
.. _auto-load safe-path: https://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading-safe-path.html

LLDB
----

This repository also includes a script that customizes LLDB's standard ``print`` command to print a ``bson_t`` or ``bson_t *`` as JSON::

    (lldb) print b
    (bson_t) $0 = {"x": 1, "y": 2}

The custom ``bson`` command provides more options::

    (lldb) bson --verbose b
    len=19
    flags=INLINE|STATIC
    {
      "x": 1,
      "y": 2
    }
    (lldb) bson --raw b
    '\x13\x00\x00\x00\x10x\x00\x01\x00\x00\x00\x10y\x00\x02\x00\x00\x00\x00'

Type ``help bson`` for a list of options.

The script requires a build of libbson with debug symbols, and an installation of `PyMongo`_. Install PyMongo with::

  python -m pip install pymongo

If you see "No module named pip" then you must `install pip`_, then run the previous command again.

Create a file ``~/.lldbinit`` containing::

  command script import /path/to/mongo-c-driver/lldb_bson.py

If you see "bson command installed by lldb_bson" at the beginning of your LLDB session, you've installed the script correctly.

.. _PyMongo: https://pypi.python.org/pypi/pymongo
.. _install pip: https://pip.pypa.io/en/stable/installing/#installing-with-get-pip-py)

Debug assertions
----------------

To enable runtime debug assertions, configure with ``-DENABLE_DEBUG_ASSERTIONS=ON``.
