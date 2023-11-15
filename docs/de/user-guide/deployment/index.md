---
title: Deployment
page_id: "130.0"
---

## Deployment

A developed application is deployed in the production environment (or test environment) and it is about to run there.

Although it's easier if the source code is built in the production environment, in general the production environment and the build machine are separate. For building, the computer needs to have the same OS/Library installed as in the production environment. The binary for the release can then be built. The binary and its related files, which all will be created, are then transferred from the archive to the production environment.

### Release Mode Building

In order to build the source code in release mode, the following command should be used in the application root:

```
 $ qmake -r "CONFIG+=release"
 $ make clean
 $ make
```

A binary, that is optimized to the environment, should have been generated then.

### Deploying to the Production Environment

First, check the settings of the production environment. You should check the user *name/password* in the [*product*] section in the *database.ini* settings file and the number of the listening port in the *application.ini* file. Please ensure all these settings are correct for your environment.

The following list is a summary of folder and files needed for your application to work correctly. All the folders and files will be archived in the following directory:

* config
* db      ← not necessary if sqlite is not used
* lib
* plugin
* public
* sql

Examples of the *tar* command:

```
 $ tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
```

- Please change the tar file name accordingly.

This is then the setup of the production environment. Building and configuration of the database systems and installation of TreeFrog Framework/Qt have been completed in advance.

The tar file is copied to the production environment. Once copied, it can then be expanded by creating a directory.

```
 $ mkdir app_name
 $ cd app_name
 $ tar xvfz (directory-name)/app.tar.gz
```

To start, use the following command by specifying the application root directory (must be an absolute path):

```
 $ sudo treefrog -d  [application_root_path]
```

Some distributions may require you to have root privileges if you want to open port 80. In this example, I started the service with the sudo command.

In addition, in Linux, you will be able to activate the app automatically by making the *init.d* script. In Windows, this is possible by registering the startup. Because there are many articles on the internet about how to start the service automatically at OS boot time, I won't need to go into detail here.

The next statement shows the Stop Command for stopping the TreeFrog service.

```
 $ sudo treefrog -k stop [application_root_path]
```

### Release Mode Building on CI tools
Using CI tools to build the source code on release mode or test as close to the production environment as possible is one of the most effective ways to do.

An example usage:
```yaml
# GitHub Action
# .github/workflows/c-cpp.yml
name: C/C++ CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:

    runs-on: ubuntu-18.04

    steps:
    - uses: actions/checkout@v2

    # build treefrog
    - name: dependency install
      run: sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev qtdeclarative5-dev qtbase5-dev-tools gcc g++ make cmake
    - name: dependency db install
      run: sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1 libmongoc-dev libbson-dev
    - name: download treefrog source archive
      run: wget https://github.com/treefrogframework/treefrog-framework/archive/v1.30.0.tar.gz
    - name: expand treefrog archive 
      run: tar zxvf v*.*.*.tar.gz
    - name: configure treefrog
      run: cd treefrog-framework-*.* && ./configure --prefix=/usr/local
    - name: make treefrog
      run: cd treefrog-framework-*.* && make -j4 -C src
    - name: make install treefrog
      run: cd treefrog-framework-*.* && sudo make install -C src
    - name: make treefrog tools
      run: cd treefrog-framework-*.* && make -j4 -C tools
    - name: make install treefrog tools
      run: cd treefrog-framework-*.* && sudo make install -C tools
    - name: update share library dependency info
      run: sudo ldconfig
    - name: check treefrog version
      run: treefrog -v
    
    # build project files
    - name: execute qmake.
      run: qmake -r "CONFIG+=release"
    - name: makeing directory for build
      run: mkdir build
    - name: cmake 
      run: cd build && cmake ../
    - name: execute make
      run: cd build && make

    # package project files.
    - name: compressive archive
      run: tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
    - name: staging
      run: mkdir staging && mv app.tar.gz staging/
    - uses: actions/upload-artifact@v2
      with:
        name: Package
        path: staging
```