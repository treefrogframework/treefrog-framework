---
title: 模型(Model)
page_id: "060.0"
---

## 模型(Model)

模型(Model)是一个对象, 这个对象表现为需要返回给浏览器的抽象信息. 事实上, 在业务逻辑上它不是这么简单的. 所以, 让我们来尝试理解它.

模型(Model)保存在系统内或者外部数据库内. 从控制器(controller)角度来看, 不考虑模型(model), 你在通过HTTP响应的方式访问数据库里期望的信息. 一旦信息已经在数据库中保存, 你将可以获得一个模型(model)的信息, 这些信息你可以传递到视图(view).

当你直接使用SQL访问数据库时, 在某些情况下编写的代码变得复杂难懂, 因此代码比较难阅读.当数据库表结构复杂的情况下更加明显.要避免这个是很困难的, Treefrog设计了一个O/R 映射系统(称为SQlObject).

在一个网页程序中CRUD(create, read, update, delete)是最基本的需求. 你所编写的SQL语句几乎都是重复的常规语句. 对于这一部分, O/R映射系统将会非常有效地工作.

我个人认为, 对于网页框架, 建议模型(Model)对象自己有一个关联的用于O/R映射的对象(这些对象将被当成的ORM对象). 默认情况下, 在Treefrog框架中每个模型(model)对象包含一个ORM对象.

    "一个模型(model)有一个ORM对象."
	
这种设计有以下好处:

* ORM类结构直观
* 能够隐藏控制器(controller)端不需要的信息.
* 能够合并一个模型(model)记录的逻辑删除和物理删除.
  ->使用这个界面去创建一个删除标识列, 然后仅仅需要在用模型(model)的remove()方法去更新.
* 因为模型(model)是一对一的, 不需要关系表. 模型(model)类的设计自由度更大了. 
  ->业务逻辑能够以很自然的方式增加到模型(model)中.

缺点就是代码量增加了一点.

##### 概要: 隐藏视图(controller)和视图(view)不需要的信息.

## 模型(Model)的API

通常, 一个类越独立就越能够重复使用. 因此, 理想的模型依应该赖的尽可能小.

在网页应用中, 数据库经常用来保存数据, 因此"模型类(model class)"与"表"相关. 需要处理各种类型的数据的网页应用必须相应的创建许多表(模型(models)). 设计数据库架构时通常需要规范化设计, 模型之间通过它们的属性将存在一个关系.

当编写模型类时, 应该知道下面的这些惯例.

* 使用texport()方法传递参数到视图(view).
    ->这相当与在下面的类中从QVariant类型设置一个变量(使用setValue()方法):
    - public default constructor
    - public copy constructor 
    - public destructor
    - Q_DECLARE_METATYPE 宏声明 (在每个头文件的尾部)

如果想更进一步了解它, 请阅读 [Qt文档](http://doc.qt.io/qt-5/qmetatype.html){:target="_blank"}.

#### 通过命令行生成器生成的模型(model)已经满足了工作的最小需求.

生成器生成的模型(model)类继承于TAbstractModel类. 我为了要使用TAbstractModel的优点, 使Model类继承了它. 还有, 处理ORM对象的实用的方法也能够使用. 本质上, 继承是为了重用, 通过继承后, 这些模型(model)根本不需要访问数据库.

##### 概要: 如果你想使用ORM对象, 你应该继承TAbstractModel类.

当生成器生成模型(model)后, 每个属性的getter/setter和等效于"create"和"read"的类的方法都已经定义好了. 下面的例子是在[教程({{ site.baseurl }}/user-guide/cn/tutorial/index.html){:target="_blank"}中生成的Blog类的片段.

```c++
static Blog create(const QString &title, const QString &body);
static Blog create(const QVariantMap &values);
static Blog get(int id);       // 通过特定的ID获得模型(model)对象
static Blog get(int id, int lockRevision); 
static QList<Blog> getAll();   // 获取所有的模型(model)对象
```

当你运行create()方法, 对象的内容将保存在数据库中.

让我们看看TAbstractModel类中定义的方法:

```c++
virtual bool create();          // 新建
virtual bool save();            // 保存(新建或者更新)
virtual bool update();          // 更新
virtual bool remove();          // 删除
virtual bool isNull() const;    // 数据库中是否是空的
virtual bool isNew() const;     // 数据库中是否已经存在
virtual bool isSaved() const;   // 数据库中是否已经保存
void setProperties(const QVariantMap &properties);
```

如果ORM对象不存在, save()方法在内部调用create()方法. 如果ORM对象存在, save()方法在内部调用 update()方法. 所以, 如果你不想区分create()和update()方法, 你可以简单的使用save()方法来调用模型(model).

这里生成的代码只是冰山上的一角. 你可以增加或修改属性, 例如从*protected*到*private*转换或者任何你喜欢的修改.

## 使用不同于表名的名字创建模型(model)名

当你使用生成器命令生成模型(model)时, 模型(model)名基于表名的格式'_'(下划线).<br>
如果你希望给模型(model)一个不同的名称, 你可以按下面的方式在命令后紧接着一个字符串:

```
$ tspawn  model  blog_entries  BlogEntry    -> 仅创建模型(model)
$ tspawn  s  blog_entries  BlogEntry   -> 创建模型(model)-视图(view)-控制器(controller)
```

## 生成一个原始模型(Model)

一个模型(Model)并不需要一个数据库表对应. 它也可以用来汇总数据并传递给视图(view).

如果你不使用生成器生成一个模型, 你可以按下面的方法声明一个类:

```c++
class T_MODEL_EXPORT Post
{
  public:
    // 包含默认的构造函数, 复制函数, 解析函数
    // 编写代码.
};
Q_DECLARE_METATYPE(Post)        // 传递给视图(view)的宏
Q_DECLARE_METATYPE(QList<Post>) // 传递列表给视图(view)的宏
```
保存它在models文件夹, 定义头文件和源文件的名字 并把文件增加到项目(*models.pro*). 