---
title: MongoDB中的对象-文档映射
page_id: "060.060"
---

## MongoDB中的对象-文档映射

MongoDB用类似于JSON格式来保存数据, 并将数据保存为一个文档. 在编程语言中, 和文档对象相关的功能被称之为**对象-文档映射(O/D 映射)**.

像在[O/R 映射](/ch/user-guide/model/or-mapping.html){:target="_blank"}中描述的一样, 在O/D 映射中这里的一个文档也与一个对象相关.

因为MongoDB文档是类似JSON格式的, JSON可能有层级结构, 但是在O/D 映射中, 一个对象不会对应文档的两层或者更多的层级. 例如, 下面的例子显示了O/D 映射中唯一支持的简单形式:

```json
{
  "name": "John Smith",
  "age": 20,
  "email": "foo@example.com"
}
```

## 设置

如果你完全不知道如何正确设置MongoDB连接, 请参考[访问MongoDB](/ch/user-guide/model/access-mongodb.html){:target="_blank"}章节.

要生成O/D 映射的类, 在应用程序的根目录下执行下面的命令. 在这个例子中, 我们建立一个叫*foo*的集合. 模型(model)的名字将会是*Foo*.

```
$ tspawn mm  foo
  created   models/mongoobjects/fooobject.h
  created   models/foo.h
  created   models/foo.cpp
  updated models/ models. pro
```

下一步是定义将要存储在文档中的数据. 我们编辑C++头文件*models/mongoobjects/fooobject.h*, 增加字符串变量*titel*和*body*.

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
  public:
    QString title;     // 加在这里
    QString body;      // 加在这里
    QString _id;
    QDateTime createdAt;
    QDateTime updatedAt;
    int lockRevision;
    enum PropertyIndex {
          Id = 0,
          CreatedAt,
          UpdatedAt,
          LockRevision,
};
 :
```

除了**_id**以外的变量都不是必须的, 所以你可以删除它们.变量*_id*等效于MongoDB中的ObjectID, 所以**不要**删除它.

- 这个对象负责访问MongoDB, 这也是为什么我们称之为"Mongo对象".

再次执行下面的命令将增加的内容反射到其他文件.

```
$ tspawn mm foo
```

键入'Y'更改所有文件.<br>
它完成了模型(model)的[CRUD](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete){:target="_blank"}.

如果想生成的骨架中包含*控制器(controllers)*和*视图(views), 可以执行下面的命令代替'tspawn mm foo'.

```
$ tspawn ms foo
```

现在, 骨架已经生成. 编译后, 试试在应用服务器(AP)中运行.

```
$ treefrog -d -e dev
```

在浏览器中访问*http://localhost:8800/foo/*, 包含列表的一个界面将显示. 使用这个界面作为新建, 编辑和删除的入口.

如你所见, 这个文件非常类似迁移文件(Rails中), 因为你通过编辑Mongo对象的类自动定义/更改了Mongo文档的布局.

## 读取Mongo对象

让我们看看骨架生成的Mongo对象类如何被读取. 我们通过使用对象ID作为关键字加载Mongo对象.

```c++
QString id;
id = ...
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
```

## 创建Mongo对象

和普通对象创建的方式一样, 通过设置对象属性创建它. 完成以后, 调用*create()*方法将会在MongoDB中创建一个新的文档.

```c++
FooObject foo;
foo.title = ...
foo.body = ...
foo.create();
```

因为对象ID是自动创建的, 请不要设置它.

## 更新Mongo对象

通过对象ID获取Mongo对象, 并设置新值. 完成以后, 调用*update()*方法将在MongoDB内容更新对象.

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.title = ...
foo.update();
```

保存文档还有一个*save()*方法.<br>
如果在MongoDB中**相关的文档不存在**它会调用*create()*方法, 如果**文档存在**则调用*update()*方法.

## 删除Mongo对象

删除Mongo对象会删除文档. 使用*remove()*方法删除对象.

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.remove();
```

#### 补充

如上所述, 可以像使用ORM对象(O/R 映射对象)一样使用Mongo对象. 从控制器(controller)的角度看, 因为模型(model)提供的功能是一样的, 它们的使用没有不同的地方. 这些对象被隐藏在模型(model)中的'private'代码块.

换句话说, 如果你不重叠地定义模型类(model class), 甚至可以同时访问MongoDB和关系型数据库, 这样可以让你非常容易的件数据保存在不同的数据库系统上.如果能正确的实现它, 你可以减少有时导致系统瓶颈的关系型数据库的负债. <br>
然而, 在发布时, 你需要考虑是否数据的性质是否可以保存在关系型数据库或保存在MongoDB中. 也许, 这个问题是你是否希望使用会话?

这样, 你可以很容易地使用不同的机制访问数据库, 这些机制能让你创建一个可扩展可伸缩的网页应用系统.

** Mongo对象类和ORM对象类的区别**

在Mongo对象类中, 你可以像下面的代码显示的那样定义**QstringList**作为实例变量:

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
  public:
    QString _id;
    QStringList  texts;
    : 
```

* 请留意: **QstringList**不能被在ORM对象类中定义.