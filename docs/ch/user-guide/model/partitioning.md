---
title: 分区
page_id: "060.040"
---

## 分区

分区用来将表A和表B放在不同的服务器上, 平衡数据库的负债.在一个网页系统中, 数据库经常是整个系统的瓶颈.

随着数据库服务器加载的表的数据量的增长(类似与磁盘I/O), 性能的损失也随之增加. 最简单的处理就是扩展内存(因为目前内存非常便宜), 然而, 这种方案有很多问题, 因此它可能不是想象中的那样简单.

能够通过增加数据库服务器, 并考虑服务器负载的分区. 然而, 这是一个棘手的问题, 服务器的扩展取决于数据库的容量. 许多关于这个方面的有用的书籍已经出版. 为此, 我将不进一步讨论它.

Treefrog框架提供了一个分区的机制, 用来方便访问在不同数据库服务器上的数据.

## 通过SQlObject分区

作为前提条件, 我们在主机A上使用表A, 主机B上使用表B.

首先, 设置不同的描述不同主机连接信息的数据库配置文件. 文件名必须依次为*databaseA.ini*和*databaseB.ini*. 它们应该位于config目录内. 对于文件的内容, 填入恰当的值. 这里是*dev*节可以定义的例子.

```ini
[dev]
DriverType=QMYSQL
DatabaseName=foodb
HostName=192.168.xxx.xxx
Port=3306
UserName= root
Password=xxxx
ConnectOptions=
```

接下来在应用配置文件(*application.ini*)中定义数据库配置文件的名字. 用空格隔开各个值并写入*DatabaseSettingFiles*.

```ini
# 指定数据库配置文件.
DatabaseSettingsFiles=databaseA.ini  databaseB.ini
```

数据库的ID像下面的例子一样,按照序列0, 1, 2...排序:

* The database ID of the host A : 0
* The database ID of the host B : 1

然后,用这个数据库ID设置SQlObject的头文件.<br>
编辑生成器上次的头文件.你也可以重写databaseId()方法返回数据库ID.

```c++
class T_MODEL_EXPORT BlogObject : public TSqlObject, public QSharedData
{
     :
   int databaseId() const { return 1; }  // returns ID 1
     :
};
```

这样, BlogObject的查询将在主机B上执行. 然后, 我们可以像往常一样使用SQlObject.

这只是2个数据库服务器的例子, 但它同样支持3个或数量更多的数据库服务器. 在配置文件中简单的增加*DatabaseSettingFiles*.

这样分区将在不改变模型(model)和控制器(controller)逻辑的情况下被应用.

## 查询分区的表

像在[SQL查询]({{ site.baseurl }}/ch/user-guide/model/sql-query.html){:target="_blank"}中讨论的一样, TSqlQuery不仅仅是用来执行一条查询, 或者不需要提取所有的值, 你在它上面使用分区.

接下来, 在创建器的第二个参数指定服务器ID:

```c++
TSqlQuery query("SELECT * FROM foo WHERE ...",  1);  // 指定数据库 ID 1
query.exec();
   :
```

这个查询将在主机B上执行.