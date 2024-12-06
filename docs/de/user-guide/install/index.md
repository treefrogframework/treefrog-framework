---
title: Install
page_id: "020.0"
---

## Install

First of all, we need to install the Qt library in advance.

On Windows and macOS, download the package from the [Qt site](http://qt-project.org/downloads){:target="_blank"} and install it.
On Linux, you can install a package for each distribution.

In case of Ubuntu:
Install the Qt libraries and dev tools:

```
 $ sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql
 libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev
 qtdeclarative5-dev qtbase5-dev-tools gcc g++ make cmake
```

Now install the DB client libraries:

```
 $ sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1 libmongoc-dev libbson-dev
```

### Installation Instructions

1. Extract the file you just have [downloaded](http://www.treefrogframework.org/en/download/){:target="_blank"}.

   The following command applies to version 'x.x.x'. Please update it appropriately.

   ```
    $ tar xvfz treefrog-x.x.x.tar.gz
   ```

2. Run build commands.

   In Windows:
   Please create a binary of two types for *release* and for *debugging*.
   Start the Qt Command Prompt and then build with the following commands. The configuration batch should be run twice.

   ```
  > cd treefrog-x.x.x
  > configure --enable-debug
  > cd src
  > nmake install
  > cd ..\tools
  > nmake install
  > cd ..
  > configure
  > cd src
  > nmake install
  > cd ..\tools
  > nmake install
   ```

   In UNIX-based OS Linux, and macOS:
   Enter the following command from the command line:

   ```
  $ cd treefrog-x.x.x
  $ ./configure
  $ cd src
  $ make
  $ sudo make install
  $ cd ../tools
  $ make
  $ sudo make install
   ```

   **Note:**
   In order to debug the TreeFrog Framework itself, please use the *configure* option.
   Now please run this command:

   ```
  ./configure --enable-debug
   ```

   The next command updates the dynamic linker runtime bindings in Linux only.

   ```
  $ sudo ldconfig
   ```

3. Create a shortcut of TreeFrog Command Prompt (Windows only).
   Right-click on the folder on which you want to create a shortcut and select "New" and then click the "Shortcut". Set the links as follows:

   ```
   C:\Windows\System32\cmd.exe /K  C:\TreeFrog\x.x.x\bin\tfenv.bat
   ```

   ('x.x.x' represents the current version you use)

   <div class="img-center" markdown="1">

   ![Create Shortcut]({{ site.baseurl }}/assets/images/documentation/shortcut_en.png "Create Shortcut")

   </div>

   Set the Shortcut name to 'TreeFrog Command Prompt'.

   <div class="img-center" markdown="1">

   ![Shortcut name]({{ site.baseurl }}/assets/images/documentation/shortcut-name_en.png "Shortcut name")

   </div>

### Configure Option

By specifying various options, you can customize to suit your environment.

Options available on Windows using "Configure option":

```
 > configure --help
 Usage: configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [C:\TreeFrog\x.x.x]
```

Options available on Linux, and UNIX-like OS:

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library
   --spec=SPEC         use SPEC as QMAKESPEC

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [/usr]

 Fine tuning of the installation directories:
   --bindir=DIR        user executables [/usr/bin]
   --libdir=DIR        object code libraries [/usr/lib]
   --includedir=DIR    C header files [/usr/include/treefrog]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```

Options available in macOS:

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library

 Fine tuning of the installation directories:
   --framework=PREFIX  install framework files in PREFIX [/Library/Frameworks]
   --bindir=DIR        user executables [/usr/bin]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog
```
