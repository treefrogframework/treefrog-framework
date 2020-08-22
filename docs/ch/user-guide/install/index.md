---
title: 安装
page_id: "020.0"
---

## 安装

首先,我们需要先安装好Qt库.

对于Windows和macOS,从[Qt 网站](http://qt-project.org/downloads){:target="_blank"} 下载并安装它.
对于Linux,你可以安装一个发布的包.

如果是Ubuntu:
安装Qt库和开发工具:

```
$ sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql
libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev
qtdeclarative5-dev qtbase5-dev-tools gcc g++ make cmake
```

现在安装数据库客户端库:

```
$ sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1
```

### 安装说明
1. 解压已[下载](http://www.treefrogframework.org/ja/%E3%83%80%E3%82%A6%E3%83%B3%E3%83%AD%E3%83%BC%E3%83%89){:target="_blank"}的文件.

   以下命令应用到版本'x.x.x'.请对于的替换.

   ```
   $ tar xvfz treefrog-x.x.x.tar.gz
   ```

2. 运行build命令.

   对于Windows:
   请为*发行(releae)*和调试(debugging)*创建两种不同类型的二进制文件.
   打开Qt命令行窗口, 然后使用以下命令Build.配置批处理文件(configure.bat)应该执行两次.

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

   在基于UNIX的操作系统Linux和macOS:
   在命令行输入以下命令:

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

   **说明:**
   为了调试Treefrog框架本身,请使用*configure*选项.
   现在请执行命令:

   ```
   ./configure --enable-debug
   ```

   下一条命令更新动态链接器运行时绑定(仅在Linux).

   ```
   $ sudo ldconfig
   ```

3.创建Treefrog命令行窗口的快捷方式(仅在Windows).
   在你想要的文件夹下点击右键, 选择"新建"然后点击"快捷方式". 设置链接目标如下:

   ```
   C:\Windows\System32\cmd.exe /K  C:\TreeFrog\x.x.x\bin\tfenv.bat
   ```

   ('x.x.x'表示当前使用的版本)

   <div class="img-center" markdown="1">

   ![创建快捷方式]({{ site.baseurl }}/assets/images/documentation/shortcut_ch.png "新建快捷方式")

   </div>

   设置快捷方式的名字为'Treefrog命令行窗口'.

   <div class="img-center" markdown="1">

   ![快捷方式名称]({{ site.baseurl }}/assets/images/documentation/shortcut-name_ch.png "快捷方式名称")

   </div>

### 配置选项

通过定义各自选项,可以进行环境的客户化设定.

在Windows上使用选项"configure option":

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

在Linux和类UNIX OS上使用选项:

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [/usr]

 Fine tuning of the installation directories:
   --bindir=DIR        user executables [/usr/bin]
   --libdir=DIR        object code libraries [/usr/lib]
   --includedir=DIR    C header files [/usr/include/treefrog]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```

在Max OS X上使用选项:

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
