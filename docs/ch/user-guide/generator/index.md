---
title: 生成器
page_id: "040.0"
---

## 生成器

在这一章节,我们将看看命名为*tspawn*的生成器命令.

## 生成应用程序框架

首先, 在我们做其他事情前需要创建一个应用程序的框架. 我们将再次使用*blogapp*创建. 从命令行输入下面的命令(在Windows中, 从Treefrog命令行窗口执行)

```
 $ tspawn new blogapp
```

当你执行完这条命令, 应用名将作为目录树的根目录.配置文件(*ini*)和项目文件(*pro*)也会生成. 目录是你已经看过的一些名字.<br>
下面的项目将作为一个文件夹生成.

* controllers
* models
* views
* heplers
* config      -配置文件
* db          - 数据库存储文件 (SQLite)
* lib 
* log         -记录文件
* plugin 
* public      -静态HTML文件, 图片,JavaScript 文件
* script 
* test
* tmp         -临时目录,例如上传的文件

## 生成骨架

骨架包括CURD操作的基本实现. 骨架包含以下部件:控制器(controller), 模型(model), *视图(view)*源代码的源代码, 还有项目文件(pro). 因此, 这些骨架构成了一个好的基础帮助你完成一个完整的开发.

为了用生成器命令*tspawn*生成一个骨架, 需要先在数据库定义一个表, 然后在配置文件(*database.ini*)中设置数据库信息.<br>
现在, 开始定义一张表.见下面的例子:

```sql
> CREATE TABLE blog (id INTEGER PRIMARY KEY, title VARCHAR(20), body VARCHAR(200));
```

如果想使用SQLite作为数据库, 你应该把数据库文件放在应用程序的根目录内.你可以在配置文件中设置数据库信息.生成器命令使用在*dev*节设置的信息.

```ini
[dev]
driverType=QMYSQL
databaseName=blogdb
hostName=
port=
userName=root
password=root
connectOptions=
```

<div class="center aligned" markdown="1">

**设置清单**

</div>

<div class="table-div" markdown="1">
| 项目           | 含义           | 说明                                                                              |
| -------------- |----------------|-----------------------------------------------------------------------------------|
| driverType     | 驱动名称       | 有以下选项:<br>- QDB2: IBM DB2<br>- QIBASE: Borland InterBase Driver<br>- QMYSQL: MySQL Driver<br>- QOCI: Oracle Call Interface Driver<br>- QODBC: ODBC Driver<br>- QPSQL: PostgreSQL Driver<br>- QSQLITE: SQLite version 3 or above |
| databaseName   | 数据库名称     | 如果是SQLite必须定义文件路径.<br>如:db/blogdb                                     |
| hostName       | 主机名称       |  留空表示*localhost*                                                              |
| port           | 端口           | 留空表示默认端口                                                                  |
| userName       | 用户名         |                                                                                   |
| password       | 密码           |                                                                                   |
| connectOptions | 连接选项       | 更多信息参看Qt文档:<br>[QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"} |

</div><br>

如果Qt SDK没有包含数据库驱动, 将不能访问数据库. 如果还没有构建, 你必须设置好驱动. 作为替代, 你可以从[下载页](http://www.treefrogframework.org/download){:target="_blank"}下载数据库驱动, 然后安装它.

当你执行完生成器命令(完成上面提到的步骤), 骨架就会生成. 每条命令都应该在应用程序的根目录下执行.

```
$ cd blogapp
$ tspawn scaffold blog
driverType: QMYSQL
databaseName: blogdb
hostName:
Database open successfully
  created  controllers/blogcontroller.h
  created  controllers/blogcontroller.cpp
  :
```

<br>
##### 概要: 在数据库中定义好表结构, 然后使用生成器生成骨架.**

### 模型名称(Model-Name)/控制器名称(Controller-Name)和表名(Table Name)的关系

生成器会基于表名生产类的名字.规则如下:

```
　表名             模型名          控制器名                 SQL对象名
　blog_entry   ->  BlogEntry       BlogEntryController      BlogEntryObject
```

请注意下划线会被删掉, 后面的第一个字母会改成大写的. 可以完全忽略单词单数和复数形式之间的区别.

## 生成器子命令

这里是tspawn命令的用法规则:

```
$ tspawn -h
usage: tspawn <subcommand> [args]
Available subcommands:
  new (n)  <application-name>
  scaffold (s)  <model-name>
  controller (c)  <controller-name>
  model (m)  <table-name>
  sqlobject (o)  <table-name>
```

如果你使用"controller", "model", "sqlobject"作为子命令, 将只会生成"controller", "model" 和 "SqlObject".

### 列

Treefrog没有升级数据库的功能或者更改管理数据库表结构的机制. 基于以下原因,我认为它是不重要的:

1. 如果我实现升级功能, 用户会有额外的学习成本.
2. 这些是关于DB操作的完整功能的SQL 知识.
3. 在Treefrog, 可以在表更改后重新生成ORM对象类.(不幸, 可能也会影响到Model类)
4. 我认为对SQL命令进行框架差异管理是没有什么价值的。

你是否同意这些观点?

## 名称转换

Treefrog 有类名和文件名转换功能. 生成器按照下面的条款和规定生成类名和文件名.

#### 控制器名称的转换

控制器类名是"表名+Controller"控制器的类名永远使用大写字母打头, 不要使用下划线分隔单词, 而是将下划线后的单词首个字母换成大写的.<br>
以下类名是很好的例子帮助理解如何转换.

* BlogController
* EntryCommentController

这些文件保存在controller文件夹内. 文件夹内的文件名是全部小写的, 类名加上对应的扩展名(.cpp 或 .h).

#### 模型名称的转换

和控制器的方式一样, 模型名称永远使用大写字母打头, 不要使用下划线分隔单词, 而是将下划线后的单词首个字母换成大写的. 例如下面的类名:

* Blog
* EntryComment

这些文件保存在models文件夹内.和控制器一样, 这些文件名全部是小写的.模型名后加上文件扩展(.cpp 或 .h).
和Rails不同, 我们不使用单词单数和复数形式的转换.

#### 视图名称的转换

模版文件按照"控制器名称+扩展名"的文件名形式生成, 文件名全部是小写的, 在"views/控制器名"目录内. 扩展名取决于选择的模版系统.

同时, 当你构建视图然后输出的源文件在"views/_src"文件内. 你会注意到这些文件已经全部转换成了C++ 代码模版. 当文件编译后, 会生成一个view的共享库.

#### CRUD

CRUD包括了网页应用的四个主要功能. 这个名称来自于四个单词的首字符"新建(Create)", "读取(Read)", "更新(Update)"和"删除(Delete)".
当你新建一个骨架, 生成器命令生成如下名称的代码:

<div class="center aligned" markdown="1">

**CURD对应表**

</div>

<div class="table-div" markdown="1">

|  | Action        | Model                               | ORM       | SQL       |
|--|---------------|-------------------------------------|-----------|-----------|
| C| create        | create() [static]<br>create()       | create()  | INSERT    |
| R| index<br>show | get() [static]<br>getAll() [static] | find()    | SELECT    |
| U| save          | save()<br>update()                  | update()  | UPDATE    |
| D| remove        | remove()                            | remove()  | DELETE    |

</div><br>

## 关于 T_CONTROLLER_EXPORT宏

生成器生成的控制器类将会增加到宏T_CONTROLLER_EXPORT.

在Windows上, 控制器是一个单独的DLL文件, 但是为了在DLL外可使用这些类和功能, 我们需要使用关键字__declspec(称为dllexport)定义它. T_CONTROLLER_EXPORT宏会用这个关键字完成替换.<br>
然而, 在Linux和Mac OS X下安装, T_CONTROLLER_EXPORT没有定义任何内容, 因为关键字__declspec是不需要的.

```
 #define T_CONTROLLER_EXPORT
```

通过这种方式, 同样的代码就能够支持多平台了.