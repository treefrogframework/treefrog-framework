
安装需求
------------
 - Windows, Linux, macOS, 或符合POSIX的Unix-like OS
 - Qt 5.4或更高版本
 - C 编译器和C++14编译器
 - Make工具

在Linux上, 你可以通过'apt-get'或'yum'安装.
如果是Ubuntu:
  安装Qt库和开发工具.

      $ sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql
           libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5
           qtbase5-dev qtdeclarative5-dev qtbase5-dev-tools gcc g++ make

  安装数据库客户端库文件. (可选)

      $ sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1

  想更好理解请看http://doc.qt.io/qt-5/sql-driver.html


安装
------------
1. 解压下载的tar.gz文件.

2. 运行build命令.
  Linux, macOS, 或 Unix-like OS:
  运行下面的命令.

       $ cd treefrog-x.x.x
       $ ./configure
       $ cd src
       $ make
       $ sudo make install
       $ cd ../tools
       $ make
       $ sudo make install

  Windows:
  分别构建两个库,release和debug.
  在Qt命令行窗口运行下面的命令.

    Visual Studio:
    在Qt命令行窗口运行vcvarsall.bat完成环境设置, 如下.

         > cd "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64"
         > vcvars64.bat

         > cd (treefrog-x.x.x)
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

	  或者在完成编辑Qt环境后,双击运行build.bat.

3. 为Treefrog命令行窗口创建快捷方式(仅在Windows下).
  定位程序位置:

      C:\Windows\System32\cmd.exe /K C:\TreeFrog\x.x.x\bin\tfenv.bat
      ("x.x.x" 表示版本)

  * 在Treefrog命令行窗口内构建网页应用和运行Treefrog命令,如tspawn或reefrog.


 开始体验吧!
