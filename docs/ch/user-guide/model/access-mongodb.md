---
title: 访问MongoDB
page_id: "060.050"
---

## 访问MongoDB

MongoDB是一个开源的文档存储数据库. 它是NoSql体系中的一个.

在关系型数据库中管理数据, 你必须先定义一个表(架构), 但是在MongoDb中将不再需要做这些. 在MongoDB中, 数据是通过一种叫做"文档(Documents)"的类似于JSON格式(BSON)进行存储的, 数据集(set)通过"集合(Collection)"管理. 在系统中, 每一个文档(document)分配一个唯一的ID(ObjectID).

这里是关系型数据库和MongoDB分层结构的对比:

<div class="table-div" markdown="1">

| MongoDB    | 关系型数据库      | 说明          |
|------------|-------------------|---------------|
| Database   | Database          | 术语一样      |
| Collection | Table             |               |
| Document   | Record            |               |

</div><br>

## 安装

使用下面的命令重新安装框架:

```
$ tar xvzf treefrog-x.x.x.tar.gz
$ cd treefrog-x.x.x
$ ./configure --enable-mongo
$ cd src
$ make
$ sudo make install
$ cd ../tools
$ make
$ sudo make install
```

-x.x.x表示当前下载的版本.

## 关键设置


假设MongoDB已经被安装并且服务器已经在运行. 然后你可以使用生成器生成应用的框架.

为了和MongoDB服务器通讯, 让我们设置连接信息. 首先, 编辑*config/application.ini*的这一行.

```ini
MongoDbSettingsFile=mongodb.ini
```

然后编辑*config/mongodb.ini*, 指定主机名和数据库名. 像SQL数据库的配置文件一样, 配置文件的内容分成3节, *dev*, *test*,和*product*.

```ini
[dev]
DatabaseName=foodb        # 数据库名
HostName=192.168.x.x      # 主机名或者IP地址
Port=
UserName=
Password=
ConnectOptions=           # unused
```

现在, 在MongoDB运行的情况下, 让我们在应用程序根目录先执行下面的命令来检查设置.

```
$ tspawn --show-collections
DatabaseName: foodb
HostName:     localhost
MongoDB opened successfully
-----------------
Existing collections:
```

如果成功了, 将会显示上面的内容.

网页应用可以同时访问MongoDB和SQL数据库. 这样能够使网页应用在系统复杂增加的情况下灵活响应.

## 新建文档

要访问MongoDB服务器, 使用*TmongoQuery*对象. 指定集合名称为构建器的参数来创建实例.

MongoDB文档用QVariantMap对象来表示. 设置对象的键值对, 然后用insert()方法插入到MongoDB的尾部.

```c++
#include <TMongoQuery>
--- 
TMongoQuery mongo("blog");  // 对blog集合进行操作
QVariantMap doc;

doc["title"] = "Hello";
doc["body"] = "Hello world.";
mongo.insert(doc);   // 插入新文档
```

在内部, 当insert()方法被调用时,分配了一个唯一的ObjectID.

### 补充

从这个例子中可以看到, 开发者不需要关心连接/断开MongoDB, 因为连接管理是框架自己来处理的. 通过复用连接的机制, 连接/端口的数量可以保存在少量.

## 读取文档(Document)

当你搜索文档时, 并且有符合设置条件时, 必须一个一个地将返回的文档传递到一个QVariantMap中. 请小心, 这里必须使用QVariantMap, 因为查询条件也表示为QVariantMap.

下面的例子创建了一个包含两个查询条件的Criteria对象, 然后作为find()方法的参数被传递. 假设这里有不知道一个i文档符合查询条件, 我们使用*while*语句循环列出可用的文档(documents).

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;

criteria["title"] = "foo";  // 设置查询条件
criteria["body"] = "bar";  // 设置查询条件

mongo.find(criteria);    // 执行查询
while( mongo. next()){
    QVariantMap doc = mongo.value(); // 获得一个文档
    // 处理过程
}
```

- 两个查询条件被AND操作符连接起来.

如果你仅仅查找一个符合条件的文档, 你可以使用findOne()方法.

```c++
QVariantMap doc = mongo.findOne(criteria);
``` 

这下面的例子设置'num'的查询条件. 只有符合'num'的值大于10的文档才匹配. 为了实现它, 使用**$gt**作为查询条件对象的比较运算符.

```c++
QVariantMap criteria;
QVariantMap gt;
gt.insert("$gt", 10);
criteria.insert("num", gt);   // 设置查询条件
mongo.find(criteria); // 执行查询
   :
```

**比较运算符:**

* **$gt**: 大于
* **$gte**: 大于等于
* **$lt**: 小于
* **$lte**: 小于等于
* **$ne**: 不等于
* **$in**: 在结果中
* **$nin**: 不再结果中

### OR运算符

使用逻辑运算符OR **$or** 操作符连接查询条件

```c++
QVariantMap criteria;
QVariantList orlst;
orlst << c1 << c2 << c3;  // 3个查询条件
criteria.insert("$or", orlst);
   :
```

如上所述, *TmongoQuery*的查询条件是用一个那个*QVariantMap*类型的对象表示的. 在MongoDB中, 查询条件用JSON表示, 所以当执行一条查询时, QvariantMap对象会被转换成JSON对象. 因此你可以指定所有MongoDB支持的操作符(这些操作符已经根据它们的规则被进行了正确的描述). 有效的查询然后就变的可能了.

MongoDB还提供了更多的运算符. 要更深入的了解请查看[MongoDB文档](http://docs.mongodb.org/manual/reference/operator/nav-query/){:target="_blank"}.

## 更新文档

我们将从MongoDB服务器读取一个文档然后更新它. 如update()方法所示, 我们将更新匹配匹配的文档.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["title"] = "foo"; // 设置查询条件
QVariantMap doc = mongo.findOne(criteria);   // 获得一个文档
doc["body"] = "bar baz";               // 更改文档的内容

criteria["_id"] = doc["_id"];          // 设置ObjectID为查询条件
mongo.update(criteria, doc);
```

这里有非常重要的地方需要注意, 如果有几个文档匹配查询条件, 为了确保文档被更新, 增加*ObjectID*到查询条件.

此外, 如果你像更新匹配查询条件的所有文档, 你可以使用updateMulti()方法.

```c++
mongo.updateMulti(criteria, doc);
```

## 删除文档

如果想删除一个文档, 定义对象的ID作为条件.

```c++
criteria["_id"] = "517b4909c6efa89aed288706";  // 根据ObjectID删除对象
mongo.remove(criteria);
```

你还可以删除所有匹配的文档.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["foo"] = "bar";
mongo.remove(criteria);    // 删除
```
