---
title: Test
page_id: "120.0"
---

## Test

In the process of application development, testing is very important. Testing requires checking by repeating, and it is a boring process. For this reason, it might be very useful to automate this process.

## Unit Test of the Model

In this session, we will try to check if the model works the right way. The test framework follows TestLib attached by Qt (please check out the [documentation](http://qt-project.org/doc/qt-5.0/qttestlib/qtest-overview.html){:target="_blank"} for more details).

Let's test the Blog model code that we made in the [tutorial](/en/user-guide/tutorial/index.html){:target="_blank"}. Make a common library for the model in advance. At first, we will create a working directory in the *test* directory.

```
 $ cd test
 $ mkdir blog
 $ cd blog
```

We will try to create the test case for creating and reading of the Blog model.<br>
For example, let's set the name of the implementing test as follows: *TestBlog*. The source code with the following content is saved as a file named *testblog.cpp*.

```c++
#include <TfTest/TfTest>
#include "models/blog.h"    //  include the model class

class TestBlog : public QObject
{
    Q_OBJECT
private slots:
    void create_data();
    void create();
};

void TestBlog::create_data()
{
    // definition of test data
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("body");

    // adding to test data
    QTest::newRow("No1") << "Hello" << "Hello world.";
}

void TestBlog::create()
{
    // acquisition of test data
    QFETCH(QString, title);
    QFETCH(QString, body);

    // logic of the test
    Blog created = Blog::create(title, body);
    int id = created.id();
    Blog blog = Blog::get(id);  // Getting model ID

    // verification of result execution
    QCOMPARE(blog.title(), title);
    QCOMPARE(blog.body(), body);
}

TF_TEST_MAIN(TestBlog)   // specify the class name you created
#include "testblog.moc"  // charm. Make the extension .moc
```

As supplemental comment, among this, create() method can do the test, and QCOMPARE macro can check the real returning value. The create_data() method works as passing test data to the create_data() method.
The rule is always to put '_data' at the end of the method name.

In this example, I am doing the following in create_data() method.

* QTest::addColumn() function: Define the name and type of test data.
* QTest::newRow() function: Add test data.

The following is done in the create() method.

* Fetch the test data.
* Run the test logic.
* Verify that the result is correct.

Next, create a project file to make the *Makefile*. The file name is *testblog.pro*, saving the contents as the following.

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
 SOURCES = testblog.cpp      # Specifying the file name
```

After you save the project file, you can create a binary by running the following command in its directory:

```
 $ qmake
 $ make
```

Next, some little configuration needs to be done for the testing process.<br>
Because of the need to refer to the various configuration files, the test command requires a symbolic link to the config directory. Its location should be directly below of the test command. When SQLite is used for the database, we need to make a symbolic link to the *db* directory as well.

```
 $ ln -s  ../../config  config
 $ ln -s  ../../db  db
```

If you use Windows, an exe file of the test is created in the *debug* directory, so that a symbolic link is created there. Please be careful: it is NOT a shortcut!
To create a symbolic link, you must run the command from the command prompt launched with administrator privileges.

```
 > cd debug
 > mklink /D  config  ..\..\..\config
 > mklink /D  db  ..\..\..\db
```

Furthermore, take the path to the common library including the Blog model.<br>
In the case of Linux, set the environment variable as follows:

```
 $ export  LD_LIBRARY_PATH=/path/to/blogapp/lib
```

If you use Windows, add the setting to PATH variables like this:

```
 > set PATH=C:\path\to\blogapp\lib;%PATH%
```

Then check the connection information for the database. In the unit test, the connection information in the test section in the database configuration file (*database.ini*) is used.

```
[test]
DriverType=QMYSQL
DatabaseName=blogdb
HostName=
Port=
UserName=root
Password=
ConnectOptions=
```

The configuration is now complete. Next, the test needs to be executed. If the test was a throughout success, you can see the following message on the screen:

```
$ ./testblog
Config: Using QtTest library 5.5.1, Qt 5.5.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 5.4.0 20160609)
PASS   : TestBlog::initTestCase()
PASS   : TestBlog::create(No1)
PASS   : TestBlog::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestBlog *********
```

In the case of Windows, please execute the test on the TreeFrog Command Prompt.<br>
If, however, the result is not what was expected, you will see the following message.

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

Make a test case for each each model. Then please do the test. The key to a good Web application development is to be sure that the model is working properly.
