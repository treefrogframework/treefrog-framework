---
title: ERB
page_id: "070.010"
---

## ERB

ERB原本是一个用来嵌入Ruby脚本到文本文档中的一个库. 在Rails被作为一个模版引擎, 你可以将代码嵌入到HTML中的标签<% ...%>中.

用同样的方式, Treefrog框架也使用<% ...%>标签来嵌入C++代码.为了方便, 我也把它称之为ERB.

首先, 我将确保*development.ini*中的配置项目是按如下配置的, 除非你有更改默认设置, 它应该是这样.

```
TemplateSystem=ERB
```

然后, 当你用命令生成器生成一个视图(view), ERB模版格式将生成.生成的模版的文件名必须遵循全部是小写的规则.

```
views/Controller-name/Action-name.erb    (小写)
```

如果你希望增加一个新的模版, 请遵守同样的名字转换规则.

## 视图(view)和操作(action)的关系

我将要来回顾一下视图(view)和操作(action)之间的关系. 举例, 当Foo控制器(controller)的bar操作(action)不带参数地调用render()方法, 下面的模版的内容将被输出.

```
views/foo/bar.erb
```

如果你带参数调用render()方法, 对应的模版内容将被输出.

你可以随便增加模版文件. 但要记住, 一旦当你增加了一个或者几个模版后, 你必须在view目录下执行一次下面的命令.

```
$ cd views 
$ make qmake
```

那就是为了增加新模版需要做的所有动作,新模版会被增加到makefile进行构建.
记得要反射新的模版, 这些模版已经增加到共享库中.

##### 概要:在你增加了一个新模版文件后, 使用"make qmake"命令.

## 输出字符串

我们将要输出字符串"Hello world."有两种方式来实现它. 第一种, 我们可以使用下面的方法:

```
<% eh("Hello world"); %>
```

eh()方法使用HTML转义输出字符串, 这样有跨域脚本保护. 如果你不想使用转义, 使用echo()方法代替.

另外一种方法是使用<%= ... %>符号. 它和eh()方法输出一样的结果.

```
<%= "Hello world" %>
```

如果你不想使用转义, 使用<%== ... %>代替, 它输出和echo()一样的结果.

```
<%== "<p>Hello world</p>"  %>
```

**说明:**<br>
在eRuby原始的规范中, 使用<%= ... %>输出一个不转义的字符串. 在Treefrog中, 基于额外的代码输入很容易被错误地省略的可能性, 有这样的想法, 使用比较短的代码来标明HTML转义, 安全性会增加. 它在最近几年变成了主流.

## 显示一个默认值.

如果变量是一个空值, 让我们显示一个默认值作为替代.<br>
按下面的写法, 如果变量*str*为空, 字符"none"将显示.

```
<%= str %|% "none" %> 
```

## 使用从控制器(controller)传递的对象

为了显示从控制器(controller)使用texport()方法传递的对象, 首先用tfetch()宏或者T_FETCH宏声明类型和变量的名字. 我们将把这个操作当成是'获取(fetch)'.

```
<% tfetch(Blog, blog); %>
<%= blog.title %>       ->输出blog.title的值
```

已经获取(fetch)的变量(对象)可以在同一文件中被使用.换句话说, 它变成了局部变量.

你可能会问是否获取(fetch)是否每次使用都必须执行一次, 不过你可以看到并不需要在每一次想要使用这个变量时都执行获取(fetch)处理.

在函数中获取(fetch)的对象是一个局部变量.因此, 如果我们获取(fetch)同样的对象两次, 我们同样的变量我们会定义两次, 在编译时会发生错误. 当然, 如果将代码块分割, 它不会出现编译错误, 但是这样做没有多大意义.

##### 概要:仅使用一次fetch.

如果导出的对象类型是int和Qstring, 你可以仅仅使用tehex()函数输出.获取(fetch)的处理就可以绕过了, 像这样.

```
<% tehex(foo); %>
```

将tehex()函数理解为eh()方法和fetch()方法的组合. 获取(fetching)和局部变量有点区别, 像你在这里看到的, 函数变量(此例中的*foo*)不需要定义就可输出值.

如果不想使用HTML转义处理, 使用techoex()函数, 它是echo()方法和fetch()方法的组合. 同样地, 变量没有定义.

此外, 有另外一种方法导出对象. 你可以使用代码<=$ .. %>. 注意'$'(美元)和'='(等于)中间不能有空格.
它们输出同样的结果.

```
<%=$ foo %>
```

代码已经变得相当简单了.
这意味着你可以用'=$'代码替换tehex()方法.
类似地, <% techoex(...); %>可以用<==$ .. %>符号重写.

概括地说, 要导出int和Qstring类型的对象, 我认为使用<=$ .. %>符号输出比较好, 出发你希望仅仅输出一次(举例, 使用获取(fetch)处理).

**概要: 使用\<=\$ .. %>导出不是仅仅输出一次的对象.**

## 如何注释

下面的例子是如何写注释. 像HTML一样什么都不输出. 不过, 注释的内容会写在C++代码中.

```
<%# comment area %>
```

在C++代码被放置在views/_src文件后, 它会被编译. 如何你想查看C++代码, 可以在这里查看.

##包含文件

如果你在ERB模版中使用一个类, 例如模型(model), 需要像C++一样在包含头文件. 请注意, 它不是自动包含的.
继续包含这些:

```
<%#include "blog.h" %>
```

请注意'%'和'#'中间不能有空格. 在这个例子中, 'blog.h'文件被包含.

记住模版会转换成C++代码, 所以不要忘记在模版中包含它们.

## 循环

让我们使用循环作用在一个列表上.例如, 处理一个由Blog对象构成的bloglist列表, 它看起来像这样(如果是一个导出的对象, 先提前获取(fetch)它):

```
<% QListIterator<Blog> i(blogList);
while ( i.hasNext() ) {
     const Blog &b = i.next();  %>
     ...
<% } %>
```

你可以使用Qt的foreach语句让代码更短一些:

```
<% foreach (Blog b, blogList) { %>
    ...
<% } %>
```

这看起来比较像C++.

## 新建\<a\>标签

要创建\<a\>标签, 使用linkTo()方法:

```
<%== linkTo("Back", QUrl("/Blog/index")) %>
              ||
<a href="/Blog/index">Back</a>
```

在linkTo()方法中, 其它参数也可被指定, 更多信息请查看[API 参考](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}

你也可以使用url()方法指定一个URL. 用控制器(controller)名称作为第一个参数, 操作(action)名作为第二个参数.

```
<%== linkTo("Back", url("Blog", "index")) %>
               ||
<a href="/Blog/index/">Back</a>
```

如果模版在同一个控制器(controller)上, 可以使用urla()方法并仅仅指定操作(action)名:

```
<%== linkTo("Back", urla("index")) %>
               ||
<a href="/Blog/index/">Back</a>
```

使用脚本, 链接和确认对话框可以这样写:

```
<%== linkTo(tr("Delete"), urla("remove", 1), Tf::Post, "confirm('Are you sure?')") %>
               ||
<a href="/Blog/remove/1/" onclick="if (confirm('Are you sure?')) { 
var f = document.createElement('form');
         :  (omission)
         f.submit();
         } return false;">Delete</a>
```

让我们增加一个标签的属性, 使用THtmlAttribute类:

```
<%== linkTo("Back", urla("index"), Tf::Get, "", THtmlAttribute("class", "menu")) %>
               ||
<a href="/Blog/index/" class="menu">Back</a>
```

你可以使用一个短的a()方法来生成同样的THtmlAttribute输出:

```
<%== linkTo("Back", urla("index"), Tf::Get, "", a("class", "menu")) %>
```

如果有不止一个属性, 使用'\|'运算符:

```
a("class", "menu") | a("title", "hello")
            ||
class="menu" title="hello"
```

随便说一句, 还有一个LinkTo()方法的别名anchor()方法, 可以用同样的方式使用它们.

此外, 还有更多可以使用的方法, 请参见[API文档](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}

## 表单

我们将从浏览器表单post数据到服务器. 下面的例子, 我们使用formTag()方法post数据到同一个控制器(controller)的create操作(action).

```
<%== formTag(urla("create"), Tf::Post) %>
...
...
</form>
```

你可能会认为你也可以通过编写form标签的方式来实现它, 而不是使用formTag()方法, 不过当你使用这个方法时, 它意味框架可以做跨域访问(CSRF)的监测.从安全的角度看, 我们应这样做.要打开跨域访问伪造(CSRF)检测, 请在配置文件application.ini中设置项目:

```
EnableCsrfProtectionModule=true
```

Treefrog框架将视跨域的post请求为非法. 然而, 在开发中有各种重复的测试, 你可以临时关闭这个保护.

关键跨域访问伪造(CSRF)监测的更多信息, 请查看[安全]({{ site.baseurl }}/ch/user-guide/security/index.html){:target="_blank"}

## 布局(Layout)

布局是将网站设计中常用的模块勾勒出来的模版.它用在页眉区域,菜单区域,页脚区域, 在布局(layout)中不能随意地放置HTML元素.

当控制器(controller)请求视图(view)时有4种方式, 如下:

1. 为每个操作(action)设置布局(layout).
2. 为每个控制器(controller)设置布局(layout).
3. 使用默认布局(layout).
4. 不使用布局(layout).

在绘制视图(view)时仅仅只有一个布局(layout)被使用, 但不同的布局(layout)也可以被使用, 如果上面列出的布局(layout)有一个比较高的优先级. 因此, 举例, 这意味着不是"为每个控制器(controller)设置布局"的规则被使用, "为每个操作(action)设置布局"有更高的优先级.

让我们举一个非常简单的布局(layout)作为例子:它用扩展名.erb保存. 布局(layout)的保存目录是view/layouts文件夹.

```
<!DOCTYPE HTML>
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=UTF-8" />
<title>Blog Title</title>
</head>
...
<%== yield() %>
...
</ html>
```

这里重要的部分是这一行; <%== yield(); %>. 这一行输出模版的内容. 换句话说, 当render()方法被调用时, 模版的内容将合并到布局(layout)中.

像这样使用布局(layout), 你可以将网站常用的设计放在一起, 例如页面区域, 页脚区域. 更改网站的设计变得容易多少了, 因为需要更改的就是布局(layout).

##### 概要:布局(layout)就是网站设计的页面轮廓.

现在, 我将描述设置布局(layout)到每个方法上.

**1. 为每个操作(action)设置布局**

当调用操作(action)时, 可以将布局(layout)的名字作为render()方法的第二个参数.
这里是simplelayout.erb布局的使用例子.

```c++
render("show", "simplelayout");
```

现在, show模版的内容将合并到simplelayout布局中, 然后作为响应返回.

**2. 为每个控制器(controller)设置布局**

在控制器的构造时, 调用setLayout()方法, 指定布局(layout)名作为参数.
```c++
setLayout("basiclayout");  // 将使用basiclayout.erb布局
```

**3. 使用默认布局(layout)**

文件appliactionerb.erb是默认的布局(layout)文件. 如果你没有定义特别的布局(layout), 将使用这个默认的布局.

**4. 不使用布局(layout)**

如果上面三种情况都不符合, 将不使用布局(layout). 此外, 如果你想指定一个布局(layout)不要使用, 使用操作(action)调用函数setLayoutDisabled(true).

## 内容块(partial)模版

如果你浏览一个网站, 你会经常发现一个页面的一个区域是固定的, 它在多个页面上总是显示相同的内容. 它可能是一个广告区域, 或者一些工具栏.

在像这样的网页应用中工作, 除了上面已经讨论过的在一个布局中(layout)包括一个区域外, 还有一种方法来共享内容, 这种方法将内容剪切下来放到一个"内容块(partial)"模版中.

首先, 将希望在多个页面显示的部分剪切下来, 然后将它作为一个模版保存在views/partial文件夹中,后缀名用.erb.然后我们可以使用renderPartial()方法将它的内容绘制出来.

```
<%== renderPartial("content") %>
```

*content.erb*的内容将嵌入到原始的模版中, 然后输出. 在内容块(partial)模版中, 也可以像原始的模版一样输出对象的值.

从它们都是合并的实质上, 布局(layout)和内容块(partial)模版应该是非常类似的. 只是用不同的方式来使用它们.

我的意见是定义那些总是出现在页面中的页眉和页脚区域为布局(layout), 使用额外的内容块(partial)模版定义那些经常出现但不是总是出现在页面中的部分是一种好的方式.

##### 概要:定义固定显示的区域为布局(layout), 不固定显示的区域为内容块(partial).