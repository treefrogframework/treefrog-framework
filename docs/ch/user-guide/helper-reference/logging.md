---
title: 记录
page_id: "080.050"
---

## 记录

网页应用将会输出四个记录:

<div class="table-div" markdown="1">

|记录Log           | 文件名File Name    | 内容Content                                                                                                                                                    |
|------------------|--------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 应用程序记录     | app.log            | 记录网页应用程序. 开发者输出会被记录在这里. 关于输出方法见下方.                                                                                                |
| 访问记录         | access.log         | 记录从浏览器来的访问. 包括访问静态文件.                                                                                                                        |
| TreeFrog框架记录 | treefrog.log       | 记录Treefrog系统. 系统输出,如错误.                                                                                                                             |
| 查询记录         | query.log          | 发送到数据库执行的查询记录. 在配置文件中指定SqlQueryLog的文件名. 当停止输出时, 刷新它. 因为记录在不停输出时会有开销, 运行正式的网页应用时关闭输出是一个好主意. |

</div><br>

## 输出应用程序记录

应用程序记录用来记录网页应用. 这里可以输出应用记录的几种类型:

* tFatal() 致命错误
* tError() 错误
* tWarn() 警告
* tInfo() 信息
* tDebug() 调试
* tTrace() 追踪

参数可以像格式化字符串和变量的printf格式一样传递. 举例, 像这样:

```c++
tError("Invalid Parameter : value : %d", value);
```

然后, 下面的记录将会输出到*log/app.log*文件:

```
2011-04-01 21:06:04 ERROR [12345678] Invalid Parameter : value : -1
```

在格式化字符串的尾部不需要换行符.

## 更改记录的布局

通过设置FileLogger能够改变记录输出的布局. 布局参数在配置文件*logger.ini*中.

```ini
# Specify the layout of FileLogger.
#  %d : date-time
#  %p : priority (lowercase)
#  %P : priority (uppercase)
#  %t : thread ID (dec)
#  %T : thread ID (hex)
#  %i : PID (dec)
#  %I : PID (hex)
#  %m : log message
#  %n : newline code
FileLogger.Layout="%d %5P [%t] %m%n"
```

当记录被生成后, 日期和时间将会插入到布局中的'%d'的位置.
日期格式在FileLogger.DateTimeFormat参数定义. 可以指定的格式和QDateTime::toString()的参数是一样的. 更详细的信息请参考[Qt文档](http://doc.qt.io/qt-5/qdatetime.html){:target="_blank"}.

```ini
# 指定FileLogger的日期-时间格式, 也可以参考QDateTime类参考
FileLogger.DateTimeFormat="yyyy-MM-dd hh:mm:ss"
```

## 更改记录级别

在*logger.ini*中可以使用下面的参数更改输出的级别:

```ini
# 输出的记录等于或高于这个的优先级.
FileLogger.Threshold=debug
```

此例, 记录的级别是高于debug.

##### 概要: 使用tDebug()函数输出调试信息(开发中需要).