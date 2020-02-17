---
title: 测试
page_id: "120.0"
---

## 测试

在开发的过程中, 测试是非常重要的. 测试需要重复检查, 它是一个烦人的过程. 出于这个原因, 自动处理这个过程就变得非常有用了.

## 模型(model)的单元测试

在这一节总, 我们将尝试检查模型(model)是否工作在正确的方式. 测试框架使用了Qt的TestLib(更多详细信息,请查看[文档](http://qt-project.org/doc/qt-5.0/qttestlib/qtest-overview.html){:target="_blank"}).

让我们测试[教程](/ch/user-guide/tutorial/index.html){:target="_blank"}中生成的Blog模型(model)的代码. 先提前为模型(model)生成一个共享库. 首先, 我们在*test*目录下创建工作目录.

```
$ cd test
$ mkdir blog
$ cd blog
```

我们将尝试创建生成和读取Blog模型的测试用例.<br>
例如, 我们设置实现测试的名字为*TestBlog*. 下面内容的源代码保存在名为*testblog.cpp*的文件中.

```c++
#include <TfTest/TfTest>
#include "models/blog.h"    //  包含模型类

class TestBlog : public QObject
{
    Q_OBJECT
    private slots:
void create_data();
    void create();
};

void TestBlog::create_data()
{
    // 定义测试数据
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("body");
    //增加测试数据
    QTest::newRow("No1") << "Hello" << "Hello world.";
}

void TestBlog::create()
{
    // 获取测试数据
    QFETCH(QString, title);
    QFETCH(QString, body);
    //测试的逻辑
    Blog created = Blog::create(title, body);
    int id = created.id();
    Blog blog = Blog::get(id);  // 获取模型 ID
    //检查结果的执行
    QCOMPARE(blog.title(), title);
    QCOMPARE(blog.body(), body);
}
TF_TEST_MAIN( TestBlog)// 指定你创建的类名
#include "testblog.moc"  // 宏. Make .moc扩展
```

作为补充说明, create()方法可以执行这个测试, QCOMPARE宏可以检查实际返回的值. create_data()方法传递测试数据.
规则是在方法名后名添加'_data".

在这个例子, 我在create_data()方法中执行下面的内容.

* QTest::addColumn() function: 定义名字和测试数据的类型.
* QTest::newRow() function: 添加测试数据.

下面是在create()方法中执行的.

* 获取测试数据
* 执行测试逻辑
* 检查结果是否正确.

接下来, 创建一个项目文件来生成*Makefile*. 文件名是*testblog.pro*. 保存下面的内容.

```
TARGET = testblog
TEMPLATE = app
CONFIG += console debug c++14
CONFIG -= app_bundle
QT += network sql testlib
QT -= gui
DEFINES += TF_DLL
INCLUDEPATH += ../..
LIBS += -L../../lib -lmodel
include(../../appbase.pri)
SOURCES = testblog.cpp      # 指定文件名
```

在你保存项目文件后, 你可以通过下面的命令在目录生成一个二进制文件.

```
$ qmake
$ make
```

接下来, 要执行测试过程,一些配置需要完成.
因为需要引用各种配置文件, 测试命令需要一个配置目录的符号连接. 它的位置应该直接在测试命令下. 当使用SQLite数据库时, 我们也需要生成一个符号连接到*db*文件夹.

```
$ ln -s  ../../config  config
$ ln -s  ../../db  db
```

如果你使用window, 一个测试的exe文件在*debug*文件夹内生成, 故在那里生成符号连接. 请注意: 它不是一个快捷方式.
要创建一个符号连接, 必须有管理员权限从命令行窗口运行命令.

```
> cd debug
> mklink /D  config  ..\..\..\config
> mklink /D  db  ..\..\..\db
```

还有, 将Blog模型的路径增加到共享库路径.<br>
在Linux中, 设置环境变量如下:

```
$ export  LD_LIBRARY_PATH=/path/to/blogapp/lib
```

如果使用Windows, 将设置添加到PATH变量,如下:

```
> set PATH=C:\path\to\blogapp\lib;%PATH%
```

然后检查数据库的连接信息.在测试单元中, 在数据库配置文件(*database.ini*)的test节中的连接信息被使用.

```
[test]
DriverType=QMYSQL
DatabaseName=blogdb
HostName=
Port=
UserName= root
Password=
ConnectOptions=
```

配置现在完成了. 接下来, 测试需要被执行. 如果测试从头到尾都是成功的, 你可以在屏幕上看到下面的信息.

```
$ ./testblog
Config: Using QtTest library 5.5.1, Qt 5.5.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 5.4.0 20160609)
PASS   : TestBlog::initTestCase()
PASS   : TestBlog::create(No1)
PASS   : TestBlog::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestBlog *********
```

在Windows中, 请在Treefrog命令行串口执行命令.<br>
然而, 如果结果不是期望的那样, 你将会看到如下信息.

```
********* Start testing of TestBlog *********
Config: Using QtTest library 5.5.1, Qt 5.5.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 5.4.0 20160609)
PASS   : TestBlog::initTestCase()
FAIL!  : TestBlog::create(No1) Compared values are not the same
   Actual   (blog.body()): "foo."
   Expected (body): "Hello world."
   Loc: [testblog.cpp(35)]
PASS   : TestBlog::cleanupTestCase()
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestBlog *******
```

为每个模型生成一个测试用例, 然后执行这个测试.一个好的网页应用开发的关键是确保模型(model)正确工作.
