---
title: JSON/XML 响应
page_id: "050.050"
---

## JSON/XML 响应

还有一些这样的例子, 为了给外部的应用提供信息, 数据需要按照XML或者JSON格式输出, Ajax应用就是这样的一个例子.

## 用JSON格式发送Model的内容

如果外部应用使用JavaScript, JSON格式非常容易处理. 在Treefrog框架中处理JSON, 需要Qt 5或者更高版本.
下面是从控制器(controller)发送JSON对象的一个例子.

```c++
 Blog blog = Blog::get(10);
 renderJson(blog.toVariantMap());
```

就这么简单. 生成器生成的模型(model)已经提供了toVariantMap()方法来转换QVariantMap对象. 有个重要的地方需要注意, 这方法将转换一个模型(model)所有的属性, 因此如果需要取消一些信息, 就需要分开实现.

用List格式发送所有的Blog对象

```
 renderJson( Blog::getAllJson() );
```

还是这么简单! 然而, 请注意如果在blog数据库中有大量的记录, 可能导致意外结果.

除此之外, 还有下面的方法可以使用. 请查看[API 参考](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}.

```c++
 bool renderJson(const QJsonDocument &document);
 bool renderJson(const QJsonObject &object);
 bool renderJson(const QJsonArray &array);
 bool renderJson(const QVariantMap &map);
 bool renderJson(const QVariantList &list);
 bool renderJson(const QStringList &list);
```

## 用XML格式发送Model的内容

用XML格式发送模型(model)内容与用JSON格式发送模型(model)内容没有多少不同. 使用下面的其中一个方法可实现:

```c++
 bool renderXml(const QDomDocument &document);
 bool renderXml(const QVariantMap &map);
 bool renderXml(const QVariantList &list);
 bool renderXml(const QStringList &list);
```

如果这些输出不符合你的需求, 你可以实现一个新的模版. 如何实现它在[视图(view)]({{ site.baseurl }}/ch/user-guide/view/index.html){:target="_blank"}章节中有描述, 但是不要忘记了在仅在控制器中设置content type.

```c++
 setContentType("text/xml")
```