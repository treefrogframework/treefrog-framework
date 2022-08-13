:man_page: mongoc_installing

Installing the MongoDB C Driver (libmongoc) and BSON library (libbson)
======================================================================

The following guide will step you through the process of downloading, building, and installing the current release of the MongoDB C Driver (libmongoc) and BSON library (libbson).

Supported Platforms
-------------------

The MongoDB C Driver is `continuously tested <https://evergreen.mongodb.com/waterfall/mongo-c-driver>`_ on a variety of platforms including:

- Archlinux
- Debian 9.2, 10.0
- macOS 10.14
- Microsoft Windows Server 2008, 2016
- RHEL 6.2, 7.0, 7.1, 8.2
- Ubuntu 16.04, 18.04
- Clang 3.4, 3.5, 3.7, 3.8, 6.0
- GCC 4.8, 4.9, 5.4, 6.3, 8.2, 8.3
- MinGW-W64
- Visual Studio 2013, 2015, 2017
- x86, x86_64, ARM (aarch64), Power8 (ppc64le), zSeries (s390x)

Install libmongoc with a Package Manager
----------------------------------------

Several Linux distributions provide packages for libmongoc and its dependencies. One advantage of installing libmongoc with a package manager is that its dependencies (including libbson) will be installed automatically. If you choose to install libmongoc from distribution packages, use the package manager to confirm the version being installed is sufficient for your needs.

The libmongoc package is available on recent versions of Debian and Ubuntu.

.. code-block:: none

  $ apt-get install libmongoc-1.0-0

On Fedora, a mongo-c-driver package is available in the default repositories and can be installed with:

.. code-block:: none

  $ dnf install mongo-c-driver

On recent Red Hat systems, such as CentOS and RHEL 7, a mongo-c-driver package is available in the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repository. To check which version is available, see `https://apps.fedoraproject.org/packages/mongo-c-driver <https://apps.fedoraproject.org/packages/mongo-c-driver>`_. The package can be installed with:

.. code-block:: none

  $ yum install mongo-c-driver

On macOS systems with Homebrew, the mongo-c-driver package can be installed with:

.. code-block:: none

  $ brew install mongo-c-driver


.. _installing_libbson_with_pkg_manager:

Install libbson with a Package Manager
--------------------------------------

The libbson package is available on recent versions of Debian and Ubuntu. If you have installed libmongoc, then libbson will have already been installed as a dependency. It is also possible to install libbson without libmongoc.

.. code-block:: none

  $ apt-get install libbson-1.0-0

On Fedora, a libbson package is available in the default repositories and can be installed with:

.. code-block:: none

  $ dnf install libbson

On recent Red Hat systems, such as CentOS and RHEL 7, a libbson package
is available in the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repository. To check
which version is available, see `https://apps.fedoraproject.org/packages/libbson <https://apps.fedoraproject.org/packages/libbson>`_.
The package can be installed with:

.. code-block:: none

  $ yum install libbson

Build environment
-----------------

Build environment on Unix
^^^^^^^^^^^^^^^^^^^^^^^^^

Prerequisites for libmongoc
~~~~~~~~~~~~~~~~~~~~~~~~~~~

OpenSSL is required for authentication or for TLS connections to MongoDB. Kerberos or LDAP support requires Cyrus SASL.

To install all optional dependencies on RedHat / Fedora:

.. code-block:: none

  $ sudo yum install cmake openssl-devel cyrus-sasl-devel

On Debian / Ubuntu:

.. code-block:: none

  $ sudo apt-get install cmake libssl-dev libsasl2-dev

On FreeBSD:

.. code-block:: none

  $ su -c 'pkg install cmake openssl cyrus-sasl'

Prerequisites for libbson
~~~~~~~~~~~~~~~~~~~~~~~~~

The only prerequisite for building libbson is ``cmake``. The command lines above can be adjusted to install only ``cmake``.

Build environment on macOS
^^^^^^^^^^^^^^^^^^^^^^^^^^

Install the XCode Command Line Tools:

.. code-block:: none

  $ xcode-select --install

The ``cmake`` utility is also required. First `install Homebrew according to its instructions <https://brew.sh/>`_, then:

.. code-block:: none

  $ brew install cmake

.. _build-on-windows:

Build environment on Windows with Visual Studio
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Building on Windows requires Windows Vista or newer and Visual Studio 2010 or newer. Additionally, ``cmake`` is required to generate Visual Studio project files.  Installation of these components on Windows is beyond the scope of this document.

Build environment on Windows with MinGW-W64 and MSYS2
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Install MSYS2 from `msys2.github.io <http://msys2.github.io>`_. Choose the x86_64 version, not i686.

Open the MingGW shell with ``c:\msys64\ming64.exe`` (not the msys2_shell). Install dependencies:

.. code-block:: none

  $ pacman --noconfirm -Syu
  $ pacman --noconfirm -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
  $ pacman --noconfirm -S mingw-w64-x86_64-extra-cmake-modules make tar
  $ pacman --noconfirm -S mingw64/mingw-w64-x86_64-cyrus-sasl

Configuring the build
---------------------

Before building libmongoc and/or libbson, it is necessary to configure, or prepare, the build.  The steps to prepare the build depend on how you obtained the source code and the build platform.

Preparing a build from a release tarball
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most recent release of libmongoc and libbson, both of which are included in mongo-c-driver, can be `downloaded here <https://github.com/mongodb/mongo-c-driver/releases/latest>`_. The instructions in this document utilize ``cmake``'s out-of-source build feature to keep build artifacts separate from source files. While the ``$`` prompt is used throughout, the instructions below will work on Linux, macOS, and Windows (assuming that CMake is in the user's shell path in all cases).  See the subsequent sections for additional platform-specific instructions.

The following snippet will download and extract the driver, and configure it:

.. parsed-literal::

  $ wget https://github.com/mongodb/mongo-c-driver/releases/download/|version|/mongo-c-driver-|version|.tar.gz
  $ tar xzf mongo-c-driver-|version|.tar.gz
  $ cd mongo-c-driver-|version|
  $ mkdir cmake-build
  $ cd cmake-build
  $ cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..

The ``-DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF`` option is recommended, see :doc:`init-cleanup`. Another useful ``cmake`` option is ``-DCMAKE_BUILD_TYPE=Release`` for a release optimized build and ``-DCMAKE_BUILD_TYPE=Debug`` for a debug build. For a list of all configure options, run ``cmake -L ..``.

If ``cmake`` completed successfully, you will see a considerable amount of output describing your build configuration. The final line of output should look something like this:

.. parsed-literal::

  -- Build files have been written to: /home/user/mongo-c-driver-|version|/cmake-build

If ``cmake`` concludes with anything different, then it is likely an error occurred.

mongo-c-driver contains a copy of libbson, in case your system does not already have libbson installed. The configuration will detect if libbson is not installed and use the bundled libbson.

Additionally, it is possible to build only libbson by setting the ``-DENABLE_MONGOC=OFF`` option:

.. parsed-literal::

  $ cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_MONGOC=OFF ..

A build configuration description similar to the one above will be displayed, though with fewer entries. Once the configuration is complete, the selected items can be built and installed with these commands:

Preparing a build from a git repository clone
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Clone the repository and prepare the build on the current branch or a particular release tag:

.. parsed-literal::

  $ git clone https://github.com/mongodb/mongo-c-driver.git
  $ cd mongo-c-driver
  $ git checkout |version|  # To build a particular release
  $ python build/calc_release_version.py > VERSION_CURRENT
  $ mkdir cmake-build
  $ cd cmake-build
  $ cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..

Preparing a build on Windows with Visual Studio
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the Windows platform with Visual Studio, it may be necessary to specify the CMake generator to use.  This is especially important if multiple versions of Visual Studio are installed on the system or if alternate build tools (e.g., MinGW, MSYS2, Cygwin, etc.) are present on the system.  Specifying the generator will ensure that the build configuration is known with certainty, rather than relying on the toolchain that CMake happens to find.

Start by generating Visual Studio project files. The following assumes you are compiling for 64-bit Windows using Visual Studio 2015 Express, which can be freely downloaded from Microsoft. The sample commands utilize ``cmake``'s out-of-source build feature to keep build artifacts separate from source files.

.. parsed-literal::

  $ cd mongo-c-driver-|version|
  $ mkdir cmake-build
  $ cd cmake-build
  $ cmake -G "Visual Studio 14 2015 Win64" \\
      "-DCMAKE_INSTALL_PREFIX=C:\\mongo-c-driver" \\
      "-DCMAKE_PREFIX_PATH=C:\\mongo-c-driver" \\
      ..

(Run ``cmake -LH ..`` for a list of other options.)

To see a complete list of the CMake generators available on your specific system, use a command like this:

.. parsed-literal::

 $ cmake --help

Executing a build
-----------------

Building on Unix, macOS, and Windows (MinGW-W64 and MSYS2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. parsed-literal::

  $ cmake --build .
  $ sudo cmake --build . --target install

(Note that the ``sudo`` command may not be applicable or available depending on the configuration of your system.)

In the above commands, the first relies on the default target which builds all configured components.  For fine grained control over what gets built, the following command can be used (for Ninja and Makefile-based build systems) to list all available targets:

.. parsed-literal::

  $ cmake --build . help

Building on Windows with Visual Studio
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the project files are generated, the project can be opened directly in Visual Studio or compiled from the command line.

Build using the CMake build tool mode:

.. code-block:: none

  $ cmake --build . --config RelWithDebInfo

Visual Studio's default build type is ``Debug``, but we recommend a release build with debug info for production use. Now that libmongoc and libbson are compiled, install them. Components will be installed to the path specified by ``CMAKE_INSTALL_PREFIX``.

.. code-block:: none

  $ cmake --build . --config RelWithDebInfo --target install

You should now see libmongoc and libbson installed in ``C:\mongo-c-driver``

For Visual Studio 2019 (16.4 and newer), this command can be used to list all available targets:

.. parsed-literal::

  $ cmake --build . -- /targets

Alternately, you can examine the files matching the glob ``*.vcxproj`` in the ``cmake-build`` directory.

To use the driver libraries in your program, see :doc:`visual-studio-guide`.

Generating the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Install `Sphinx <http://www.sphinx-doc.org/>`_, then:

.. code-block:: none

  $ cmake -DENABLE_MAN_PAGES=ON -DENABLE_HTML_DOCS=ON ..
  $ cmake --build . --target mongoc-doc

To build only the libbson documentation:

.. code-block:: none

  $ cmake -DENABLE_MAN_PAGES=ON -DENABLE_HTML_DOCS=ON ..
  $ cmake --build . --target bson-doc

The ``-DENABLE_MAN_PAGES=ON`` and ``-DENABLE_HTML_DOCS=ON`` can also be added as options to a normal build from a release tarball or from git so that the documentation is built at the same time as other components.

Uninstalling the installed components
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are two ways to uninstall the components that have been installed.  The first is to invoke the uninstall program directly.  On Linux/Unix:

.. code-block:: none

  $ sudo /usr/local/share/mongo-c-driver/uninstall.sh

On Windows:

.. code-block:: none

  $ C:\mongo-c-driver\share\mongo-c-driver\uninstall.bat

The second way to uninstall is from within the build directory, assuming that it is in the exact same state as when the install command was invoked:

.. code-block:: none

  $ sudo cmake --build . --target uninstall

The second approach simply invokes the uninstall program referenced in the first approach.

Dealing with Build Failures
^^^^^^^^^^^^^^^^^^^^^^^^^^^

If your attempt to build the C driver fails, please see the `README <https://github.com/mongodb/mongo-c-driver#how-to-ask-for-help>` for instructions on requesting assistance.

Additional Options for Integrators
----------------------------------

In the event that you are building the BSON library and/or the C driver to embed with other components and you wish to avoid the potential for collision with components installed from a standard build or from a distribution package manager, you can make use of the ``BSON_OUTPUT_BASENAME`` and ``MONGOC_OUTPUT_BASENAME`` options to ``cmake``.

.. code-block:: none

  $ cmake -DBSON_OUTPUT_BASENAME=custom_bson -DMONGOC_OUTPUT_BASENAME=custom_mongoc ..

The above command would produce libraries named ``libcustom_bson.so`` and ``libcustom_mongoc.so`` (or with the extension appropriate for the build platform).  Those libraries could be placed in a standard system directory or in an alternate location and could be linked to by specifying something like ``-lcustom_mongoc -lcustom_bson`` on the linker command line (possibly adjusting the specific flags to those required by your linker).
