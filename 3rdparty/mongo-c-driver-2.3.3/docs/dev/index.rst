``mongo-c-driver`` Development Documentation
============================================

These pages detail information related to the development of the mongo-c-driver
project. These are for project developers, not end users of the C driver
library.

To view these pages in their rendered HTML form, run the following command from
the mongo-c-driver project directory:

.. code-block:: console

   $ make -C docs/dev serve

.. rubric:: Contents:

- :doc:`Releasing the MongoDB C Driver <releasing>` - Instructions on performing
  a release of the C driver libaries.

.. toctree::

   debian
   earthly
   deps
   files

.. Add the `releasing` page to a hidden toctree. We don't want to include it
   directly in a visible toctree because the top-level sections would render inline
   as top-level links, which we do not want.
.. toctree::
   :hidden:

   releasing


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

.. Hidden, because it is not currently relevant to this project:
.. * :ref:`modindex`
