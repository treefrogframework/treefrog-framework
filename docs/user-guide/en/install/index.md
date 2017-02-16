---
title: Install
page_id: "020.0"
---

## Install

Please install the Qt library in advance.

On Windows and Mac OS X, download a package from the [Qt site](http://qt-project.org/downloads){:target="_blank"} and install it.
On Linux, you can install a package of each distribution.

Case of Ubuntu :
  Install Qt libraries and dev tools.

```
 $ sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql 
 libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev 
 qtdeclarative5-dev qtbase5-dev-tools gcc g++ make
```

 Install DB client libraries.

```
 $ sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1
```

### Installation Instructions

1. Extract the file you downloaded.

   The following applies to version 'x.x.x'. Please update as appropriate.  
   
   ```
    $ tar xvfz treefrog-x.x.x.tar.gz
   ```

2. Run build commands. 
 
   In Windows:
   Please create a binary of two types for the release and for debugging.
   Start the Qt Command Prompt, and then build with the following command. The configuration batch should be run twice.

   ```
  > cd treefrog-x.x.x
  > configure --enable-debug
  > cd src
  > mingw32-make install
  > cd ..\tools
  > mingw32-make install
  > cd ..
  > configure
  > cd src
  > mingw32-make install
  > cd ..\tools
  > mingw32-make install
   ```

   In UNIX-based OS Linux, and Mac OS X:   
   Enter the following from the command line.

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

   Note: In order to debug the TreeFrog Framework itself, please use the configure option.
   Run Type

   ```
  ./configure --enable-debug
   ```

   Updates dynamic linker runtime bindings in Linux only.

   ```
  $ sudo ldconfig
   ```  
 
3. Create a shortcut of TreeFrog Command Prompt (Windows only).
   Right-click on the folder in which you want to create the shortcut, and select ⋅⋅⋅"New" – and then click the "shortcut". Set the links as follows;

   ```
C:\Windows\System32\cmd.exe /K  C:\TreeFrog\x.x.x\bin\tfenv.bat
   ```

   x.x.x is version.

### Configure option

By specifying various options, you can customize to suit your environment.
 
Options available on Windows using “Configure option” :

```
 > configure --help
 Usage: configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [C:\TreeFrog\x.x.x]
```
  
Options available on Linux, and UNIX-like OS :

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [/usr]

 Fine tuning of the installation directories:
   --bindir=DIR        user executables [/usr/bin]
   --libdir=DIR        object code libraries [/usr/lib]
   --includedir=DIR    C header files [/usr/include/treefrog]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```

Options available in Max OS X :

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information

 Fine tuning of the installation directories:
   --framework=PREFIX  install framework files in PREFIX [/Library/Frameworks]
   --bindir=DIR        user executables [/usr/bin]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```