:man_page: mongoc_create_indexes

Creating Indexes
================

To create indexes on a MongoDB collection, execute the ``createIndexes`` command
with a command function like :symbol:`mongoc_database_write_command_with_opts` or
:symbol:`mongoc_collection_write_command_with_opts`. See `the MongoDB
Manual entry for the createIndexes command
<https://docs.mongodb.com/manual/reference/command/createIndexes/>`_ for details.

.. warning::

   The ``commitQuorum`` option to the ``createIndexes`` command is only
   supported in MongoDB 4.4+ servers, but it is not validated in the command
   functions. Do not pass ``commitQuorum`` if connected to server versions less
   than 4.4. Using the ``commitQuorum`` option on server versions less than 4.4
   may have adverse effects on index builds.

Example
-------

.. literalinclude:: ../examples/example-create-indexes.c
   :language: c
   :caption: example-create-indexes.c
