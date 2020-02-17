---
title: SQL查询
page_id: "060.020"
---

## SQL查询

需要读取一个简单的记录时SQlObject的功能是够用. 但是当处理情况比较复杂时, 例如多个表的读取, 你可能希望直接使用SQL查询语句. 这个框架能够通过使用占位符生成一个安全的查询.

下面是使用占位符执行ODBC格式的SQL查询的一个例子.

```c++
TSqlQuery query;
query.prepare("INSERT INTO blog (id, title, body) VALUES (?, ?, ?)");
query.addBind( 100). addBind( tr(" Hello")). addBind( tr(" Hello world"));
query.exec(); // 执行查询
```

下面是命名的占位符:

```c++
TSqlQuery query;
query.prepare("INSERT INTO blog (id, title, body) VALUES (:id, :title, :body)");
query.bind(":id", 100).bind(":title", tr("Hello")).bind(":body", tr("Hello world"));
query. exec();// 执行查询
```

如何从查询结果中获取数据与Qt的QSqlQuery类相同.

```c++
TSqlQuery query;
query.exec("SELECT id, title FROM blog");  // 执行查询
while (query.next()) {
    int id = query.value(0).toInt(); //  转换第二个字段为整形类型
    QString str = query.value(1).toString(); // 转换第二个字段为字符串类型
    // 处理过程
}
```

TSqlQuery类也可以同样的方法, 因为它继承于Qt的[QSqlQuery类](https://doc.qt.io/qt-5/qsqlquery.html){:target="_blank"}.

##### 概要: 任何情况下都可以使用占位符创建查询.

事实上, 可以在[查询记录]({{ site.baseurl }}/ch/user-guide/helper-reference/logging.html){:target="_blank"}中查看已经被执行的任何查询.

## 从文件中读取一个查询

因为源代码中的查询在每次的编写或者修改后都需要执行编译, 你可能会感到在应用开发的过程中显得很麻烦. 为了减轻这种麻烦, 这里有机制可以将查询语句写在独立分离的文件中, 然后在运行时进行加载(译者:可以在开发完成后再写入源文件中).

文件存放在sql目录中(虽然,这个目录可以通过*application.ini*修改).*insert_blog.sql*是临时的, 将在下面描述它的内容.

```sql
INSERT INTO blog (id, title, body) VALUES (?, ?, ?)
```

下面是源代码. 使用load()方法读取*insert_blog.sql*文件.

```c++
TSqlQuery query;
query.load("insert_blog.sql")
query. addBind( 100). addBind( tr(" Hello")). addBind( tr(" Hello world"));
query. exec(); // 执行查询
```

缓存工作在load()方法的内部(但是仅在线程使用 [MPM]({{ site.baseurl }}/ch/user-guide/performance/index.html){:target="_blank"}模式时. 只有第一次使用时从文件中读取查询, 然后将使用缓存, 所以它运行得非常快.
文件完成更新后, 为了读取查询语句,我们需要重启服务器.

```
$ treefrog -k abort ;   treefrog -d  -e dev
```

或者像下面这样:

```c++
$ treefrog- k restart
``` 

## 从查询结果获得ORM对象

在上面的方法中, 必须从查询的结果获取每个字段的值, 然而, 单条记录可以用下面的方式提取成ORM对象.

使用TSqlQueryMapper对象执行查询. 然后对结果使用迭代器提取ORM对象. 为了选择所有的字段, 在SELECT语句中定义'blog.*'是非常重要的.

```c++
TSqlQueryORMapper<BlogObject> mapper;
mapper.prepare("SELECT blog.* FROM blog WHERE ...");
mapper. exec();  // 执行查询
TSqlQueryORMapperIterator<BlogObject> it(mapper);
while (it.hasNext()) {
    BlogObject obj = it.next();
    // 处理过程
     :
}
```

如果仅需要提取一个ORM对象, 可以使用execFirst()方法取得结果.