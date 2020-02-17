---
title: O/R映射
page_id: "060.010"
---

## O/R映射

模型(model)在内部通过O/R映射对象访问数据库. 映射对象是一个基本的一对一关系, 被当作ORM对象. 可以通过下图来表示:<br>

<div class="img-center" markdown="1">

![ORM]({{ site.baseurl }}/assets/images/documentation/orm.png "ORM")

</div>

一条记录对应一个对象.虽然经常有一对多的关系.<br>
因为模型(model)是一个要返回浏览器的集合信息, 你需要理解如何访问和生成数据库的ORM对象.

以下DBMS驱动是支持的(等同于Qt支持的驱动):

* MySQL
* PostgreSQL
* SQLite
* ODBC
* Oracle
* DB2
* InterBase

按照这些描述, 让我们比对数据库和面对对象语言术语的对应关系.

<div class="center aligned" markdown="1">

** 术语对应表**

</div>

<div class="table-div" markdown="1">

| 面向对象        | 关系型数据库RDB    |
|-----------------|--------------------|
| 类Class         | 表Table            |
| 对象Object      | 记录Record         |
| 属性Property    | 字段Field          |

</div><br>

## 数据库连接信息

数据库的连接信息定义在配置文件(*config/database.ini*). 配置文件的内容分成3节: *product*, *test*,和*dev*. 在网页应用启动命令字符串(treefrog)指定选项(-e), 可以切换数据库.此外, 你可以增加一个节.

参数可以通过下面的方式来设置:

<div class="table-div" markdown="1">

| 参数Parameters     | 描述Description                                                                                                                                    |
|--------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| driverType         | 从这些驱动中选择Driver typeChoose from:<br>QMYSQL, QPSQL, QSQLITE, QODBC, QOCI, QDB2, QIBASE                                                       |
| databaseName       | 如果是SQLite,含有路径的数据库文件名,如*db/dbfile    Database name Specify the file path in the case of SQLite, e.g.  *db/dbfile*                   |
| hostName           | 主机名称Host name                                                                                                                                  |
| port               | 端口Port                                                                                                                                           |
| userName           | 用户名User name                                                                                                                                    |
| password           | 密码 Password                                                                                                                                      |
| connectOptions     | 连接选项 <br>Referto 更多信息参看Qt文档documentation[QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"} |
</div><br>

这样, 当你启动网页应用时, 系统将自动管理这些数据库连接. 对于开发者来说, 不需要处理数据库的连接和关闭.

在数据库创建一个表后, 在配置文件的dev节设置连接信息.

下面的节将使用[教程]({{ site.baseurl }}/ch/user-guide/tutorial/index.html){:target="_blank"}中创建的*BlogObject*类作为例子.

## 读取ORM对象

这是一个基本的操作. 它用来从一个指定的表中查找内容. 通过findByPrimaryKey()方法传递的*id*用来在表中查找匹配它的内容(*id*设置为主键). 如何查询到有匹配的, 记录的内容将被读取.

```c++
int id;
id = ...
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
```

你还可以使用findAll()方法读取所有记录.

```c++
TSqlORMapper<BlogObject> mapper;
QList<BlogObject> list = mapper.findAll();
```

你必须特别小心, 有这种可能性:当记录数量特别大时, 读取所有记录可能消耗内存. 你可以用setLimit()方法设置一个数组的上限.

ORM内部会生成SQL语句. 要查看执行了什么样的查询语句, 请查看[查询记录]({{ site.baseurl }}/ch/user-guide/helper-reference/logging/html){:target="_blank"}.

## 迭代器

如果你想一条一条的出来查询结果, 你可以使用迭代器.

```c++
TSqlORMapper<BlogObject> mapper;
mapper.find();           // 执行查询
TSqlORMapperIterator<BlogObject> i(mapper);
while (i.hasNext()) {    // 迭代器
    BlogObject obj = i.next();
    // 处理过程..
}
```

## 指定查询条件读取ORM对象

查询条件通过Tcriteria类指定. 查询*Title*字段内容为"Hello world"的单条记录, 可以通过如下方式:

```c++
TCriteria crt(BlogObject::Title, "Hello world");
BlogObject blog = mapper.findFirst(crt);
if ( !blog.isNull() ) {
    // 如果记录存在
} else {
    // 如果记录不存在
}
```

你还可以组合多个条件.

```c++
// 条件为 title = "Hello World" 并且 create_at > "2011-01-25T13:30:00"
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.add(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // 增加Add到运算符

TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findFirst(crt);
   :
```

如果希望建立一个"或"运算符, 可以使用addOr()方法.

```c++
// 条件为 title = "Hello World" 并且 create_at > "2011-01-25T13:30:00" )
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.addOr(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // 增加AddOr到运算符
   :
```

如果用addOr()方法增加查询条件, 查询条件会用括号闭合. 如果组合使用add()和addOr()方法, 使用它们时需要小心它们的顺序.

#### 注意

记住, 当一起使用AND和OR运算时, AND运算有优先权. 也就是说, 当混合使用AND和OR运算时, 先计算AND运算符, 然后在计算OR运算. 如果你希望安装顺序执行计算, 需要使用扩号.

## 创建ORM对象

按照创建普通对象的方式创建ORM对象, 然后设置它的属性. 使用create()方法插入到数据库中.

```c++
BlogObject blog;
blog.id = ...
blog.title = ...
blog.body = ...
blog.create();  // 插入到数据库中
```

## 更新一个ORM对象

为了更新ORM对象, 在读取记录前需要首先创建一个ORM对象. 一旦从数据库中取得ORM对象, 你可以设置它的属性, 然后用update()方法更新它的状态.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.title = ...
blog.update();
```

## 删除ORM对象

删除ORM对象意味着删除它的记录.<br>
先读取ORM对象,然后通过remove()方法删除它.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.remove();
```

像其他的方法一样, 你可以直接删除符合条件的记录, 不需要创建一个ORM对象.

```c++
//删除Title字段是"Hello"的记录
TSqlORMapper<BlogObject> mapper;
mapper.removeAll( TCriteria(BlogObject::Title, tr("Hello")) );
```

## 自动ID序列号

在一些数据库系统, 有字段的自动编号功能. 例如, 在MySQL的AUTO_INCREMENT属性, 或者PostgreSQL的serial类型.

Treefrog框架也设计了这种机制.也就是说, 下面的例子数字是自动分配的. 没有必要去更新或者新建这个字段.<br>

首先, 创建一个还有自动序列的字段的表. 然后, 使用生成器命令创建模型(model)后, 我们不再需要人工更新字段(这里是'id'). 

MySQL范例:

```sql
CREATE TABLE animal ( id INT PRIMARY KEY AUTO_INCREMENT,  ...
``` 

PostgreSQL范例:

```sql
CREATE TABLE animal ( id SERIAL PRIMARY KEY, ...
```

## 新建和更新时自动保存日期和时间

经常有这样的例子, 当一条记录被记录时希望能够保存其日期和时间. 因为这是个例行的实现, 字段名称可以提前根据规则设定, 到达框架可以自己自动处理.

保存记录创建时的日期和时间,可以使用*created_at*字段名. 更新记录的日期和时间,使用*updated_at*字段名代替. 为达到目的, 我们使用TIMESTAMP类型.如何有这样的字段, ORM对象将自动设置时间戳.

<div class="table-div" markdown="1">

| 项                                     | 字段名                    |
|----------------------------------------|---------------------------|
| 保存创建时的日期和时间                 | created_at                |
| 保存修改时的日期和时间                 | updated_at 或 modified_at |

</div><br>

自动保存日期和时间也可以在数据库端进行处理. 我的建议是, 虽然在数据库中也可以很好的实现, 在框架中实现更好一些.

两种方式用哪种并不重要, 我想我们都不计较, 但是用框架实现, 你可定义你想要的复杂的字段名称, 并且让数据库做其他事.

## 乐观锁

乐观锁是在要更新记录时确认记录没有被其他人更新, 而不用在数据库锁定记录. 如果已经发生了其他更新, 就会放弃更新.

使用一个自增的整形(integer)类型的字段*lock_revision*是前提条件. Lock revision在每次更新时增加. 当读取到的值与更要更新的值不相同时, 意味着已经被其他人更新了. 只有在值相同时才进行更新. 通过这样的方式, 我们能够安全的更新记录. 因为没有使用锁, 可以预期会节省了数据库系统的内存和提升了处理速度, 虽然它们是比较轻微的.

要在SQlObject中使用乐观锁的优势, 在表中增加一个整形字段,并命名为*lock_revision*. 使用生成器创建一个类.当调用TSqlObject::remove()和TSqlObject:: update()方法时, 乐观锁将激活.

##### 概要: 在表中创建一个整形的字段并命名为lock_revision.