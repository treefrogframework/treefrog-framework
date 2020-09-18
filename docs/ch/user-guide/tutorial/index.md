---
title: 教程
page_id: "030.0"
---

## 教程

让我们新建一个Treefrog应用程序.<br>
我们将尝试生成一个简单的博客系统,它可以列出(list), 查看(view)和添加(add)/编辑(edit)/删除(delete)文字.

### 生成应用程序框架

首先,我们将需要生成一个框架(各种配置文件和目录树)我们将使用"Blogapp"做为应用的名字. 从命令行执行以下命令.(在Windows请从Treefrog命令行窗口执行)

```
$ tspawn new blogapp
  created   blogapp
  created   blogapp/controllers
  created   blogapp/models
  created   blogapp/ models/ sqlobjects
  created   blogapp/ views
  created   blogapp/ views/ layouts
  created   blogapp/views/mailer
  created   blogapp/views/partial
   :
```

### 新建表

现在我们需要在数据库中创建一个表. 我们将创建title和content(body)字段. 这里是MySQL和SQLite的范例.

MySQL范例:<br>
设置字符集为UTF-8. 你也可以在生成数据库的时候定义它(确保它设置正确,见常见问题FAQ), 你可按下面的描述定义数据库的配置文件:也可以使用命令行工具在MySQL中生成数据库.

```
$ mysql -u root -p
Enter password:
mysql> CREATE DATABASE blogdb DEFAULT CHARACTER SET utf8mb4;
Query OK, 1 row affected (0.01 sec)
mysql> USE blogdb;
Database changed
mysql> CREATE TABLE blog (id INTEGER AUTO_INCREMENT PRIMARY KEY, title  VARCHAR(20), body VARCHAR(200), created_at DATETIME, updated_at DATETIME, lock_revision INTEGER) DEFAULT CHARSET=utf8;
Query OK, 0 rows affected (0.02 sec)
mysql> DESC blog;
+---------------+--------------+------+-----+---------+----------------+
| Field         | Type         | Null | Key | Default | Extra          |
+---------------+--------------+------+-----+---------+----------------+
| id            | int(11)      | NO   | PRI | NULL    | auto_increment |
| title         | varchar(20)  | YES  |     | NULL    |                |
| body          | varchar(200) | YES  |     | NULL    |                |
| created_at    | datetime     | YES  |     | NULL    |                |
| updated_at    | datetime     | YES  |     | NULL    |                |
| lock_revision | int(11)      | YES  |     | NULL    |                |
+---------------+--------------+------+-----+---------+----------------+
6 rows in set (0.01 sec)
mysql> quit
Bye
```

**SQLite范例:**<br>
我们将把数据库文件放在DB目录内.

```
$ cd blogapp
$ sqlite3 db/blogdb
SQLite version 3.6.12
sqlite> CREATE TABLE blog (id INTEGER PRIMARY KEY AUTOINCREMENT, title VARCHAR(20), body VARCHAR(200), created_at TIMESTAMP, updated_at TIMESTAMP, lock_revision INTEGER);
sqlite>. quit
```

表blog被创建, 有以下字段: id, title, body, created_at, updated_at, and lock_revision.

使用字段updated_at和created_at, Treefrog将在每次更新时自动插入日期和时间. lock_revision字段, 用于配合乐观锁使用, 需要使用整形integer创建.

#### 乐观锁(Optimistic Locking)

乐观锁被用来存储数据同时检查信息没有被其他用户更新. 因为没有实际的写锁, 你可以期待处理更快一点.
更多信息可参见O/R映射章节.

## 设置数据库信息

使用*config/database.ini*设置数据库信息.<br>
在编辑器中打开文件,在[dev]处为每个配置项输入恰当的值, 然后点击保存.

MySQL范例:

```
[dev]
DriverType=QMYSQL
DatabaseName=blogdb
HostName=
Port=
UserName=root
Password=pass
ConnectOptions=
```

SQLite范例:

```
[dev]
DriverType=QSQLITE
DatabaseName=db/blogdb
HostName=
Port=
UserName= 
Password= 
ConnectOptions=
```

一旦你正确完成了这些设置, 就可以显示数据库的表.<br>
如果所有项都被正确设置了, 将显示一条信息如下:

```
$ cd blogapp
$ tspawn --show-tables
DriverType:   QSQLITE
DatabaseName: db\blogdb
HostName:
Database opened successfully
-----
Available tables:
 blog
```

如果需要的SQL驱动没有包含在Qt SDK中, 下面的错误信息将出现:

```
QSqlDatabase: QMYSQL driver not loaded
```

如果收到此消息，则可能未安装Qt SQL驱动程序. 安装RDBM的驱动程序.

可以通过下面的命令检查哪些SQL驱动已经安装;

```
$ tspawn --show-drivers
Available database drivers for Qt:
 QSQLITE
 QMYSQL3
 QMYSQL
 QODBC3
 QODBC
```

内建的SQL驱动可以用于SQLite, 虽然也可以通过完成一点点工作来使用SQLite驱动.

## 定义一个模版系统

在Treefrog框架中, 我们可以定义Otama或者ERB作为模版系统. 我们将在*development.ini*文件中设置TemplateSystem参数.

```
TemplateSystem=ERB
  or
TemplateSystem=Otama
```

## 自动从表生成代码

从命令行执行生成器(tspawn)命令生成下面的代码. 下面的例子展示了控制器(controller), 模型(model)和视图(view)的生成. 表名作为命令的参数.

```
$ tspawn scaffold blog
DriverType: QSQLITE
DatabaseName: db/blogdb
HostName:
Database open successfully
  created   controllers/blogcontroller.h
  created   controllers/blogcontroller.cpp
  updated controllers/ controllers. pro
  created models/ sqlobjects/ blogobject. h
  created models/ blog. h
  created models/ blog. cpp
  updated models/ models. pro
  created views/ blog
    :
```

使用tspawn选项可以生成/更新模型(model)/视图(view).

tspawn命令的帮助:

```
 $ tspawn --help
 usage: tspawn <subcommand> [args]

 Type 'tspawn --show-drivers' to show all the available database drivers for Qt.
 Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.
 Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.
 Type 'tspawn --show-collections' to show all collections in the MongoDB.

 Available subcommands:
   new (n)         <application-name>
   scaffold (s)    <table-name> [model-name]
   controller (c)  <controller-name> action [action ...]
   model (m)       <table-name> [model-name]
   usermodel (u)   <table-name> [username password [model-name]]
   sqlobject (o)   <table-name> [model-name]
   mongoscaffold (ms) <model-name>
   mongomodel (mm) <model-name>
   validator (v)   <name>
   mailer (l)      <mailer-name> action [action ...]
   delete (d)      <table-name or validator-name>
```

## Build源代码

开始Build进程前, 执行下面的命令一次(仅一次), 它将会生成一个Makefile文件.

```
$ qmake -r "CONFIG+=debug"
```

一个WARNING信息将会显示,不过事实上没有影响. 接下来, 执行make命令来编译控制器(controller), 模型(model), 视图(view)和工具助手(helper).

```
$ make     (MSVC 执行'nmake' 命令代替'make')
```

如果构建成功, 4个共享库(controller, model, view, helper)将出现在lib文件夹. 默认情况下,这些生成的库的debug模式的, 不过, 你可以重新生成Makefile文件, 使用下面的命令来生成release模式的库.

生成release模式的Makefile文件:

```
$ qmake- r" CONFIG+= release"
``` 

## 启动应用服务器

在启动应用服务器(AP server)前改变应用的根目录. 服务器将会把命令执行的路径当作应用的根目录启动. 按Ctrl+c停止服务器.

```
$ treefrog -e dev
```

在Windows下, 使用*treefrog**d**.exe*启动.

```
> treefrogd.exe -e dev
```

在Windows下, 当你构建debug模式的网页应用时,使用treefrog**d**.exe启动, 当你构建release模式的网页应用时, 使用treefrog.exe启动.#### Release and debug 模式不能混着使用, 否则不能正常工作</span>.

如果希望在后台运行, 可配合任何其他需要的选项使用-d选项.

```
$ treefrog -d -e dev
```

命令选项-e出现在上面的例子中. 在使用命令跟随着一个在database.ini中已定义的**节名**前, 可以用来它来更改数据库设置. 如果没有定义节名, 系统假设命令参数是product(当项目生成时, 下面三个节名是预定义的).

<div class="table-div" markdown="1">
| 节      | 描述                  |
| --------| ----------------------|
| dev     | 用于生成代码,开发     |
| test    | 用于测试              |
| product | 用于官方版本,生产版本 |
</div>

'-e'来源于'environment'的首字母.

停止命令:

```
$ treefrog- k stop
```

终止命令(强制终止):

```
$ treefrog- k abort
```

重启命令:

```
$ treefrog- k restart
```

如果有防火墙, 确保使用的端口是允许的(默认的端口号是8800).

作为参考，以下命令显示了当前的URL路由信息.
```
 $ treefrog --show-routes
 Available controllers:
   match   /blog/index  ->  blogcontroller.index()
   match   /blog/show/:param  ->  blogcontroller.show(id)
   match   /blog/create  ->  blogcontroller.create()
   match   /blog/save/:param  ->  blogcontroller.save(id)
   match   /blog/remove/:param  ->  blogcontroller.remove(id)
```

## 浏览器访问

我们将使用浏览器访问http://localhost:8800/Blog. 一个列表界面, 如下图将会显示.

起初, 这里没有任何记录.

<div class="img-center" markdown="1">

![Listing Blog 1]({{ site.baseurl }}/assets/images/documentation/ListingBlog-300x216.png "Listing Blog 1")

</div>

当录入两条记录后, 选项show, edit 和 remove就可见了. 如你所见, 显示日文是没有问题的(译者说明:中文也没有问题, 数据库和界面都使用UTF-8字符集).

<div class="img-center" markdown="1">

![Listing Blog 2]({{ site.baseurl }}/assets/images/documentation/ListingBlog2-300x216.png "Listing Blog 2")

</div>

Treefrog使用了一种方法机制(路由系统Routing system)来实现从请求的URL到控制器(controller)的动作(action)(如同其他框架一样).<br>
已开发的源代码可以工作在其他平台上, 只要源代码被重新构建.

你点击[这里](http://blogapp.treefrogframework.org/Blog){:target="_blank"}查看一个简单的网页应用.<br>
你可以在使用一下这个应用,它将以桌面应用的平均速度响应.

## 控制器(Controller)的源码

让我们看看生成的控制器的内容.<br>
首先, 头文件这里有几个宏代码, 它们在URL发送时需要用到.

public slot的目的是声明希望发送的操作(actions)(methods).[CRUD](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete){:target="_blank"}对应的操作(actions)已被定义. 顺便说一句, 关键字slot在Qt扩展的一个功能. 更多详细信息请参考Qt文档.

```c++
class T_CONTROLLER_EXPORT BlogController : public ApplicationController
{
    Q_OBJECT
public:
    Q_INVOKABLE
    BlogController(){}
    BlogController( const BlogController& other);
    public slots:
    void index();                     // 列出所有记录
    void show(const QString &id);     // 显示记录
    void create();                    // 新建记录
    void save(const QString &id);     // 更新(保存)
    void remove(const QString &id);   // 删除一条记录
};

T_DECLARE_CONTROLLER(BlogController, blogcontroller)     //宏
```

接下来, 看看源文件.<br>
源文件代码有点长, 需要一点耐心.

```c++
#include "blogcontroller.h"
#include "blog.h"

BlogController::BlogController(const BlogController &)
    : ApplicationController()
{ }

void BlogController::index()
{
    auto blogList = Blog::getAll();  // 取得所有Blog对象的列表
    texport(blogList);               // 传递列表的值到视图(view)
    render();                        // 渲染视图 (模版template)
}

void BlogController::show(const QString &id)
{
    auto blog = Blog::get(id.toInt()) ;  // 通过主键取得Blog模型(model)
    texport(blog); 
    render();
}

void BlogController::create()
{
    switch (httpRequest().method()) { // 检查http请求的方法类型(method type)
	
        case Tf::Get:
                 render();
                 break;
				 
        case Tf::Post: {
                 auto blog = httpRequest().formItems("blog"); // 保存从'QVariantMap'类型来的'blog'变量的表单数据 
                 auto model = Blog::create(blog);             // 从POST新建对象
                 
				 if (!model.isNull()) {
                     QString notice = "Created successfully.";
                     tflash(notice);                      // 设置瞬时信息
                     redirect(urla("show", model.id()));  // 重定向到show action
                 } else {
                     QString error = "Failed to create."; // 对象创建失败
                     texport(error);
                     texport(blog);
                     render();
                 }
                 break;
        }
        default:
               renderErrorResponse(Tf::NotFound);       // 显示一个错误页面
               break;
    }
}

void BlogController::save(const QString &id)
{
    switch (httpRequest().method()) {
    case Tf::Get: {
        auto model = Blog::get(id.toInt()); // 取得一个要更新的对象
        if (!model.isNull()) {
            session().insert("blog_lockRevision", model.lockRevision()); // 设置锁版本
            auto blog = model.toVariantMap();
            texport(blog);                  // 发送blog到视图(view)
            render();
        }
        break; 
    }

    case Tf::Post: {
        QString error;
        int rev = session().value("blog_lockRevision").toInt(); // 获得lock revision
        auto model = Blog::get(id.toInt(), rev);                // 通过ID获得blog
        
        if (model.isNull()) {
            error = "Original data not found. It may have been updated/removed by another transaction.";
            tflash(error);
            redirect(urla("save", id));
            break;
        }

        auto blog = httpRequest().formItems("blog");
        model.setProperties(blog);              // 设置请求的数据
        if (model.save()) {                     // 保存对象
            QString notice = "Updated successfully.";
            tflash(notice);
            redirect(urla("show", model.id())); // 重定向到 show 操作(action)
        } else {
            error = "Failed to update.";
            texport(error);
            texport(blog);
            render();
        }
        break; 
    }

    default:
        renderErrorResponse(Tf::NotFound);
        break;
    }
}

void BlogController::remove(const QString &pk)
{
    if (httpRequest().method() != Tf::Post) {
        renderErrorResponse(Tf::NotFound);
        return;
    }

    auto blog = Blog::get(id.toInt());   // 取得Blog对象
    blog.remove();                       // 删除对象
    redirect(urla("index"));
}

// Don't remove below this line
T_REGISTER_CONTROLLER(blogcontroller)    // 宏
```

Lock revision用来实现乐观锁. 参考后续的模型(model)获取更多信息.

如你所见,你可以使用texport方法来传递数据都视图(view)(模版template). texport方法的参数是一个QVariant对象. QVariant可以是任何类型, 所以int, QString, QList和QHash可以传递任何对象. 更多关于QVariant的详细信息, 请参考Qt文档.

## 视图(View)机制

目前Treefrog已经实现了2种模版系统(template systems). 它们是Otama和ERB. 和Rails类似, ERB用来嵌入C++代码到HTML中.

生成器自动生成的视图(view)默认是ERB文件.因此, 让我们看看index.erb的内容.如你所见, C++代码包含在<%...%>中.当index操作(action)调用render方法时, index.erb的内容作为响应被返回.


```
<!DOCTYPE HTML>
<%#include "blog.h" %>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
  <title><%= controller()->name() + ": " + controller()->activeAction() %></title>
</head>
<body>
<h1>Listing Blog</h1>

<%== linkTo("New entry", urla("entry")) %><br />
<br />
<table border="1" cellpadding="5" style="border: 1px #d0d0d0 solid; border-collapse: collapse;">
  <tr>
    <th>ID</th>
    <th>Title</th>
    <th>Body</th>
  </tr>
<% tfetch(QList<Blog>, blogList); %>
<% for (const auto &i : blogList) { %>
  <tr>
    <td><%= i.id() %></td>
    <td><%= i.title() %></td>
    <td><%= i.body() %></td>
    <td>
      <%== linkTo("Show", urla("show", i.id())) %>
      <%== linkTo("Edit", urla("save", i.id())) %>
      <%== linkTo("Remove", urla("remove", i.id()), Tf::Post, "confirm('Are you sure?')") %>
    </td>
  </tr>
<% } %>
</table>
```

**接下来, 让我们看看Otama模版系统.**

Otama模版系统系统将界面逻辑从模版中完全分离出来. 模版写成HTML文件, 掩码元素作为节的开始标识插入到HTML文件中, 掩码元素会被动态改写. 界面逻辑文件, 由C++代码编写, 提供关于掩码的逻辑.

下面的范例是*index.html*, 当定义为Otama模版系统时由生成器生成. 它可以包含文件数据, 不过你将会看到, 如果你用浏览器直接打开它, 因为它使用了HTML5, 设计在没有数据的情况下完全没有崩溃.

```
<!DOCTYPE HTML>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
  <title data-tf="@head_title"></title>
</head>
<body>
<h1>Listing Blog</h1>
<a href="#" data-tf="@link_to_entry">New entry</a><br />
<br />
<table border="1" cellpadding="5" style="border: 1px #d0d0d0 solid; border-collapse: collapse;">
  <tr>
    <th>ID</th>
    <th>Title</th>
    <th>Body</th>
    <th></th>
  </tr>
  <tr data-tf="@for">               <-标记'@for'
    <td data-tf="@id"></td>
    <td data-tf="@title"></td>
    <td data-tf="@body"></td>
    <td>
      <a data-tf="@linkToShow">Show</a>
      <a data-tf="@linkToEdit">Edit</a>
      <a data-tf="@linkToRemove">Remove</a>
    </td>
  </tr>
</table>
</body>
</html>
```

一个自定义的属性'data-tf'用来打开掩码. 这个自定义数据属性是在HTML5中定义的"@"打头的字符串用来当作掩码的值.

接下来, 让我们看看index.otm对应的界面逻辑.<br>
链接到相应逻辑的掩码(mask)在上面的模版中声明(declare), 在空行前持续有效.这个逻辑包含在C++代码中.

还使用了操作符(如 == ~ =). 这些操作符控制不同的行为(更详细的信息参见后续章节)

```c++
#include "blog.h"  <-像C++代码一样include blog.h
@head_title ~= controller()->controllerName() + ": " + controller()->actionName()

@for :
tfetch(QList<Blog>, blogList);  /* 声明对象,使用从控制器(controller)传递过来的数据 */
for (QListIterator<Blog> it(blogList); it.hasNext(); ) {
     const Blog &i = it.next();        /* 参考 Blog 对象 */
     %%      /* 通常用于循环语句, 重复子元素  */
}

@id ~= i.id()   /* 将 i.id() 的值分配到内容标记为 @id 的掩码  */

@title ~= i.title()

@body ~= i.body()

@linkToShow :== linkTo("Show", urla("show", i.id()))  /* 用 linkTo() 替换子元素 */

@linkToEdit :== linkTo("Edit", urla("edit", i.id()))

@linkToRemove :== linkTo("Remove", urla("remove", i.id()), Tf::Post, "confirm('Are you sure?')")
```

Otama操作符(及其它们的组合)是非常简单的:<br>
\~ (波浪号) 掩码元素的内容设置成右侧的值,
\= 输出HTML转义, 因此~=设置元素的内容设置成右侧的值,然后转义, 如果不希望转义HTML, 可以使用~==.
\: (冒号) 更换掩码以及掩码的内容为右侧的值, 因此:==没有HTML转义地更换元素.

### 从控制器(controller)传递数据到视图(view)

如果希望在视图(view)中使用控制器(controller)中的输出(textport)对象, 必须在视图(view)中声明tfetch(macro)方法. 参数部分, 定义变量名和变量的类型. 因为它在输出(textport)前后状态几乎是一样的, 可以像使用正常的变量一样使用它. 在上面的界面逻辑中, 像实际的变量一样使用它.

这里是如何使用的例子:

```
Controller side :
 int hoge;
 hoge = ...
 texport(hoge);
View side :
 tfetch(int, hoge);
```

Otama系统, 生成基于C++代码的界面文件和模版文件. 在框架内部, tmake用来处理它. 在代码经过编译后, 生成一个view的共享库, 所以运行起来非常快.

#### HTML术语

一个HTML元素包括三个部分, 开始标签, 内容, 结束标签.例如, 这个典型的HTML元素,
"\<p\>Hello\</p\>",  \<p\> 是开始标签, Hello 是内容,  \</p\> 是结束标签.

## 模型和ORM

因为Treefrog是基于关系型(relationships)的, 模型(model)将会包含一个ORM对象(虽然你可能希望创建有2个或者更多的ORM对象), 这个关系(relationship)是Has-a. 在这个方面, Treefrog不同于其他框架, 因为默认是使用"ORM = Object Model".你不可以更改它. 在Treefrog, ORM对象包含在模型(model)对象中.

Treefrog默认会包含一个叫SqlObject的O/R映射器(mapper). 因为C++是静态的语言, 类型声明是需要的. 让我们看看生成的SqlObject文件blogobject.h.

代码中有一半是宏代码, 不过这里的字段是声明成public成员变量的. 它接近实际的结构, 但是仅能使用CRUD或者等效的方法(create, findFirst, update, remove). 这些方法定义在TSqlORMapper类和TSqlObject类.

```c++
class T_MODEL_EXPORT BlogObject : public TSqlObject, public QSharedData
{
public:
    int id {0};
    QString title;
    QString body;
    QDateTime created_at;
    QDateTime updated_at;
    int lock_revision {0};

    enum PropertyIndex {
        Id = 0,
        Title,
        Body,
        CreatedAt,
        UpdatedAt,
        LockRevision,
    };

    int primaryKeyIndex() const override { return Id; }
    int autoValueIndex() const override { return Id; }
    QString tableName() const override { return QLatin1String("blog"); }

private:    /*** Don't modify below this line ***/
    Q_OBJECT
    Q_PROPERTY(int id READ getid WRITE setid)
    T_DEFINE_PROPERTY(int, id)
    Q_PROPERTY(QString title READ gettitle WRITE settitle)
    T_DEFINE_PROPERTY(QString, title)
    Q_PROPERTY(QString body READ getbody WRITE setbody)
    T_DEFINE_PROPERTY(QString, body)
    Q_PROPERTY(QDateTime created_at READ getcreated_at WRITE setcreated_at)
    T_DEFINE_PROPERTY(QDateTime, created_at)
    Q_PROPERTY(QDateTime updated_at READ getupdated_at WRITE setupdated_at)
    T_DEFINE_PROPERTY(QDateTime, updated_at)
    Q_PROPERTY(int lock_revision READ getlock_revision WRITE setlock_revision)
    T_DEFINE_PROPERTY(int, lock_revision)
};
```

Treefrog的O/R映射器(mapper)有查询和更新主键的方法, 不过SqlObject只有一个返回primarykeyIndex()的方法. 因此, 任何有多主键的表应该更改为单主键.还能够通过Tcriteria类的设定条件进行复杂的查询. 详细信息请参加后续章节.

接下来, 让我们看看模型(model).<br>
对象每个属性的setter/getter和生成获取的静态方法已定义好. 父类TAbstractModel定义了save, remove等方法, 这样Blog类就有了CRUD方法(*create, get, save, remove*).

```c++
class T_MODEL_EXPORT Blog : public TAbstractModel
{
public:
    Blog();
    Blog(const Blog &other);
    Blog(const BlogObject &object); // 从 ORM 对象创建模型
    ~Blog();

    int id() const;      // The following lines are the setter/getter 
    QString title() const;
    void setTitle(const QString &title);
    QString body() const;
    void setBody(const QString &body);
    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    int lockRevision() const;
    Blog &operator=(const Blog &other);

    bool create() { return TAbstractModel::create(); }
    bool update() { return TAbstractModel::update(); }
    bool save()   { return TAbstractModel::save(); }
    bool remove() { return TAbstractModel::remove(); }

    static Blog create(const QString &title, const QString &body); // 创建对象
    static Blog create(const QVariantMap &values);                 // 从Hash创建对象
    static Blog get(int id);                   // 通过id获得对象
    static Blog get(int id, int lockRevision); // 通过id 和 lockRevision 获得对象
    static int count();                 // 返回blog的记录数
    static QList<Blog> getAll();        // 获得所有模型对象                              
    static QJsonArray getAllJson();     // 获得JSON方式的所有模型对象

private:
    QSharedDataPointer<BlogObject> d;   // ORM对象的指针

    TModelObject *modelData();
    const TModelObject *modelData() const;
};

Q_DECLARE_METATYPE(Blog)
Q_DECLARE_METATYPE(QList<Blog>)
```

自动生成代码的步骤并不多, 所有基本的功能已经涵盖.

当然自动生成的代码不是完美的, 真实的应用可能会更加复杂一些. 生成的代码可能不一定合适, 因此可能需要一些修改工作. 无论如何, 这个生成器可以节省一点代码编写的时间和工作.

除了以上描述的代码, 后台还提供了结合cookies篡改检查的CSRF(Cross-site request forgery跨站请求伪造) measures, 乐观锁(optimistic locking), SQL注入的令牌授权(token authentication). 如果有兴趣, 请浏览源代码.

## 视频Demo - 简单的博客应用的创建

<div class="img-center" markdown="1">

[![视频Demo - 简单的博客应用的创建](http://img.youtube.com/vi/M_ZUPZzi9V8/0.jpg)](https://www.youtube.com/watch?v=M_ZUPZzi9V8)

</div>
