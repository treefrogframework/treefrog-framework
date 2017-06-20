---
title: 插件
page_id: "080.060"
---

## 插件

在Treefrog中, 插件指动态库(共享库, DLL)用来扩展功能. 因为Treefrog有Qt插件系统, 如果标准功能不合适你可以创建一个新的插件. 目前为止, 可以创建的插件可以分为下面这些类型:

* 记录插件(用来输出记录)
* 会话存储插件(保存和读取会话)

## 如何创建插件

创建插件与如何为Qt创建插件的一模一样的. 为了说明这一点, 让我们来创建个记录插件. 首先, 我们将在*plugin*文件夹下创建一个工作目录.

```
> cd plugin 
> mkdir sample
```

- 本质上, 可以选择任何地方作为工作目录, 但是当考虑路径方案时, 像前面提到的那样创建工作目录是一个好的方式.

我们将创建一个插件封装类(此例为TLoggerPlugin). 通过继承这个作为接口的类, 然后重写一些虚拟函数.

```c++
sampleplugin.h
---
class SamplePlugin : public TLoggerPlugin
{
    QStringList keys() const;
    TLogger *create(const QString &key);
};
```

在keys()方法中, 字符串(能够被插件支持的key)将以列表的形式返回. 在create()方法中, 对应这些key的logger的实例被创建, 并且用某种方式实现后返回一个指针.

在源文件中包含这个Qt插件后, 你可以通过设置宏注册这个插件,如下:

* 第一个参数也可是任何字符串,如插件的名字
* 第二个参数是插件类名.

```c++
sampleplugin.cpp
---
#include <QtPlugin>
   :
   :
Q_EXPORT_PLUGIN2(samplePlugin, SamplePlugin)
```

接下来, 我们创建一个函数来扩展这个插件(类). 我们将创建一个logger输出记录. 按上面的做法, 再次通过继承这个类, 然后重新一些虚拟函数.

```c++
samplelogger.h
---
class SampleLogger : public TLogger
{
public:
    QString key() const { return "Sample"; }
    bool isMultiProcessSafe() const;
    bool open();
    void close();
    bool isOpen() const;
    void log(const TLog &log);
    ...
};
```

接下来的目标是项目文件(.pro). 不要忘记将"plugin"添加到CONFIG参数!

```
TARGET = sampleplugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = ..
include(../../appbase.pri)
HEADERS = sampleplugin.h \
          samplelogger.h
SOURCES = sampleplugin.cpp \
          samplelogger.cpp
```

- 使用include()包含*appbase.pri*文件是<span style="color: red">重要的</span>.

然后, 当你构建时, 你可以生成一个可加载的动态插件. 每次保存插件到plugin文件夹时, 不要有错误, 因为应用服务器(AP server)从这个目录加载插件.
更多关于[插件系统](http://doc.qt.io/qt-5/plugins-howto.html){:target="_blank"}的详细信息请参考Qt文档.

## 记录器插件

FileLogger是一个输出记录到文件的基本的记录器. 然而, 它可能不适合需求. 例如, 如果把记录像数据库一样保存或者希望交替保存记录的文件, 你可能想要使用插件机制来扩展功能.

按上面描述的那样, 在创建一个记录器插件后, 将插件放置在plug-in目录. 还有, 要让应用服务器加载需要更新*logger.ini*文件的配置信息. 将记录器的名字赋给*loggers*参数, 并用空格隔开. 下面的例子显示了它应该看起来像:

```
Loggers=FileLogger Sample
```

这样, 当你启动应用服务器时, 这些插件将被加载.

再说一次, 记录器的插件接口是一个类,如下面显示的:

* 插件接口: TLoggerPlugin
* 记录器接口: TLogger

### 关于记录器方法

要实现这个记录器, 你可以重写TLogger类的下面的方法:

* key(): 返回记录器的名字.
* open(): 加载后通过插件立即打开记录.
* close(): 关闭记录.
* log(): 输出记录. 这个方法是多线程调用的, 要使它能**线程安全thread-safe**.
* isMultiProcessSafe(): 指示在多线程处理输出一个记录时是否安全. 安全则返回true, 不安全则返回false.

关于多线程处理安全方法: 当返回*false*(表示不安全)并且应用服务器运行在多线程模式, isMultiProcessSaft()方法在输出记录之前和之后每次都调用open()/close()(导致开销增加).<br>
同时, 系统还有锁定/解锁信号保证不会有冲突. 当返回*true*时, 系统只会首先调用open()方法.

## 会话存储插件

在Treefrog中会话存储的标准方式有以下几种:

* Cookies Session: 保存到cookies中.
* DB Session: 保存到数据库中.需要创建表来实现.
* File session: 保存到文件

如果这些都不合适, 你可以创建一个继承接口类的插件.

* 插件接口: TSessionStorePlugin
* 会话存储接口: TSessionStore

按照上面描述的方法, 通过重写继承类的虚拟函数, 你可以创建一个插件. 然后, 要加载这个插件, 你可以设置在*application.ini*文件的参数*Session.StoreType*为指定的值(只能选择一个). 默认的值为cookie.