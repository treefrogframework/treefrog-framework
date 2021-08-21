---
title: 发布
page_id: "130.0"
---

## 发布

一个开发好的应用是发布到生产环境(或者测试环境)的, 它将在那里进行运行.

虽然在生产环境中构建源代码比较容易, 通常来说, 生产环境和构建的机器是分开的.构建时, 计算机需要有相同的操作系统和库安装在生产环境中. 发行版的二进制文件可以在那样的环境下构建. 二进制和所有生成的相关的文件从归档文件中转移到生产环境.

### 发行版本(release Mode)构建

要用发行版本模式构建源代码, 应该在应用程序根目录下使用下面的命令:

```
 $ qmake- r" CONFIG+= release"
 $ make clean
 $ make
```

一个根据环境优化的二进制文件然后会被生成.

### 发布到生产环境

首先, 检查生产环境的设置. 你应该检查*database.ini*配置文件[*product*]节的用户*name/password*和*application.ini*的监听端口. 请确认这些配置是符合你的环境的.

下面的清单是应用程序正常工作需要的文件夹和文件的概要.下面的目录的所有文件夹和文件应该被打包.

* config
* db      <- 如果没使用SQLite就不需要
* lib
* plugin
* public
* sql

*tar*命令的示范:

```
 $ tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
```

-请相应的更改打包文件名.

接下来是设置生产环境. 先提前构建和配置好数据库系统和安装好Treefrog/QT框架.
打包的文件复制到生产环境中. 复制完成后,可以通过创建文件夹展开它.

```
 $ mkdir app_name
 $ cd app_name
 $ tar xvfz (directory-name)/app.tar.gz
```

要启动它, 使用下面的命令指定应用程序根目录(必须是绝对路径):

```
 $ sudo treefrog -d  [application_root_path]
```

有一些发布, 如果你想打开端口80, 可能需要你有root权限. 在此例中, 我使用sudo命令启动这个服务.

此外, 在linux中, 你可以创建*init.d*脚本使程序自动激活. 在Windows中, 可以通过注册启动来实现. 因为在互联网上有很多关于如何在系统启动后自动开始服务的文章, 不需要我再详细描述了.

下一条语句显示Stop命令停止Treefrog服务.

```
 $ sudo treefrog -k stop [application_root_path]
```

### 基于CI工具的发布模式
使用CI工具在realease模式下构建源码，或者尽可能地接近生产环境进行测试，是最有效的方法之一。

一个使用实例。
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