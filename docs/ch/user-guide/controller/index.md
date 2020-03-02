---
title: 控制器
page_id: "050.0"
---

## 控制器

控制器是网页应用程序的关键类. 它接收浏览器的请求, 从模型(model)的角度调用业务逻辑, 在基本的结果上生成视图(view)的HTML, 返回请求响应.

## 定义操作(Actions)

**操作(action)**是基于请求URL的一种叫法, 它决定了调用控制器(controller)定义的何种方法.

让我们给已经生成的骨架上增加一些操作(Action). 首先, 在头文件声明的'pulic slots'部分的操作(action). 请注意, 如果你希望增加操作(action)的参数时, 这些参数应该声明成QString类型. 当你声明任何其类型时, 它们将不被识别(通过正常的方法). 最多可以定义10个参数.

```c++
class T_CONTROLLER_EXPORT FooController : public ApplicationController
{
    Q_OBJECT
     :
public slots:   // 动作(action)定义在这里
    void bar();  
    void baz(const QString &str);
     :
```

完成后, 按照往常一样实现这些操作(actions).

## 操作(action)的选择判断

当一个URL字符串被请求时, 需要决定哪个操作(action)被调用才是正确的. 默认情况下, 将使用下面的规则.

```
/controller-name/action-name/argument1/argument2/ ...
```

这些参数将被当成"操作(Action)参数".

如果使用了超过10个参数, 只有10个参数有效. 如果你希望使用11个或者更多参数, 请按照下面的方法使用Post或者URL数据参数.

这里有一些具体的例子. BlogContrller类的操作(actions)被每个用例调用.

```
/blog/show        -> show();
/blog/show/2      -> show(QString("2"));
/blog/show/foo/5  -> show(QString("foo"), QString("5")); 
```

如果省略操作(Action), 默认情况下会调用*index*操作. 这个看起来像这样:

```
/blog   -> index();
```

如果请求的URL对应的操作(action)没有定义, 将返回状态码500(内部服务器错误)给浏览器.

在大多数情况下,每个调用的操作(action)都会进行以下处理过程:

* 请求的审查
* 访问cookies和会话
* 访问上传的文件
* 调用模型(业务逻辑)
* 传递变量到视图(view)
* 请求建立HTML反馈

这些对于编程者来说, 通常意味着许多的工作和大量的代码编写. 因此, 导致的结果就是一些人编写了一些庞大而且复杂的控制器(controller). 为了防止这种情况, 请仅在模型(model)实现业务逻辑. 你应该一直保存控制器尽可能的简单.

一旦你明白了操作(action)如何调用的机制, 还能够通过URL客户化操作(action). 更多信息请参见[URL 路由](http://www.treefrogframework.org/documents/controller/url-routing){:target="_blank"}.

## 获取请求

一个HTTP请求由THttpRequest类表示.
在控制器(controller)中, 你可以用httpRequest()方法获得ThttpRequest对象.

```c++
const THttpRequest & TActionController::httpRequest () const;
```

你可以在这里获得各自数据.

## 接收请求数据

从浏览器发送的HTTP请求包含方法(method), 头(header), 主体(body). 到达服务器的数据,可以通过以下方式:

* post数据 - 使用POST方式从表单提交的数据
* URL参数(查询参数) - 附加在URL上在"?"后的参数,(格式为Key=value&...)
* Action参数 - 在Action后的参数("blog/edit/3"的第三部分)  <-见上面的内容.

接下来的例子说明了如何在控制器中获取post数据.
我们假设视图中已经有了一个<input>标签.

```
<input type="text" name="title" />
```

在服务器端要获取浏览器发送给控制器的值, 使用下面的代码:

```c++
QString val = httpRequest().formItemValue("title")
```

同时, 如果你希望将值转变为*int整形*, 可以使用Qt的toInt()方法.

如果有大量的数据项发送, 一个一个的获取显得有点麻烦. 这里有另外的一种方法一次性获取所有数据.
例如, 你可以将标签写成这样:

```
<input type="text" name="blog[title]" />
<input type="text" name="blog[body]" />
```

在控制器内, 可以这样获得数据:

```c++
 QVariantMap blog = httpRequest().formItems("blog");
 QVariant t = blog["title"];
 QVariant b = blog["body"];
  :
```
请求的数据可以表现为hash格式.

如何获取URL参数(查询参数)?这里有个例子:

```
http://example.com/blog/index?mode=normal
```

你可以使用下面的方法获得*blog*控制器(controller)的*Index*操作(action)的*mode*的值.

```
QString val = httpRequest().queryItemValue("mode");  //val ="narmal"
```

同时, 如果你想获得不是URL参数和post数据的数据, 你可以使用allParameters()和methods().

为了在应用服务器端检查请求的数据是否是要求的形式, 还提供了验证器功能. 更多信息请参见[验证器](/ch/user-guide/helper-reference/validation.html){:target="_blank"}章节.

## 传递变量到视图(view)

要传递变量到视图(view), 使用*texport(variable)*或者*T_EXPORT(variable)*宏. 你可以指定的参数类型为*QVariant*, *int*, *QString*, *QList*, 或者 *QHash*. 当然你也可以指定一个自定义的模型(model)这儿是它的用法:

```c++
 QString foo = "Hello world";
 texport(foo);
 // 或者
 int bar;
 bar = ...
 texport(bar);
```

**说明:**texport的参数必须指定为变量. 你不能直接指定一个字符串("Hello world")或者数字(例如100).

在视图(view)内要使用变量, 你必须首先用*tfetch(Type,variable)*声明变量. 更多详细内容请参见[视图(view)]({{ site.baseurl }}/ch/user-guide/view/index.html){:target="_blank"}章节.

##### 概要: 使用tfetch()传递一个对象到视图(view).

** 用户自定义的类的情况**

当你需要传递一个新的类(自定义的类)到视图(view), 请件下面的宏增加到头文件的尾部. 更多详细的信息请参见 ["新建原始模型(original model)"](/ch/user-guide/model/index/html){:target="_blank"}节.

```c++
 Q_DECLARE_METATYPE(ClassName)     // 请更换类名
```

Qt Q_DECLARE_METATYE已经提供的类不需要再声明. 但是, 如果需要传递一个模版类, 声明还是需要的, 如QHash和QList在这个列子中, 请在*helpers/applicationhelper.h*尾部添加下面的内容

```c++
    :
 Q_DECLARE_METATYPE(QList<float>)
```

Q_DECLARE_METATYPE 宏的参数是一个类名. 但是,如果你包含了一个逗号(如QHash \<Foo, Bar\>), 将会产生编译错误. 在这种情况下, 名字应该使用typedef QHash \<Foo, Bar\>来声明.

```c++
 typedef QHash<Foo, Bar> BarHash;
 Q_DECLARE_METATYPE(BarHash)
``` 

#### 导出对象

我们视导出对象到视图(view)(在texport()方法设置的对象)为"导出对象".

## 请求建立反馈内容

在处理完业务逻辑后, 处理的结果作为HTML反馈将会返回. 如果BlogController *show* 操作(action)被执行, 在*views/blog/show.xxx*中的反馈内容被模版名(扩展名基于模版系统)生成. 要请求反馈内容, 你可以使用render()方法.

```c++
 bool render(const QString &action = QString(), const QString &layout = QString());
```

如果你想反馈一个不同的模版, 指定操作(action)名为参数即可. 布局(layout)文件名可以被指定为布局参数.

方法**render()**表示"请求 view/template 建立反馈内容". 这也称之为"绘图(drawing)".

##### 概要:要渲染模版,使用 render() 方法.

### 布局(Layout)

当年创建一个网站的是, 网页页眉, 网页页脚还有一些其他的部分通常所有的页面中是通用的, 只是网页的内容不同而已. 术语布局(layout)是给这些通用的部分使用的, 模版基于这些通用的部分. 布局(layout)文件应该放在views/layouts文件夹内.

### 关于模版系统

TreeFrog目前为止采用了两种模版系统:**Otama**和**ERB**. 在ERB内, 代码通过<%...%>嵌入. 在Treefrog内, Otama系统有不同的处理方式, 因为它完全分离了逻辑(.tom)和界面模版(.html).

* ERB 使用文件扩展名: xxx.erb
* Otama 使用文件扩展名: xxx.otm和xxx.html <br> (xxx指操作(action)的名称)

## 定向到渲染一个字符串

要直接渲染一个字符串,使用 renderText()方法.

```c++
 // "Hello world" 渲染这个字符串
 renderText("Hello world");
```

默认情况下, 是不使用布局(layout)的. 如果想要使用布局, 你需要指定第二个参数为*true*.

```c++
 // 渲染字符串 "Hello world" 同时使用布局(layout)
 renderText("Hello world", true);
```

关于布局(layout)的更多详细的信息请参见 [视图(view)](/ch/user-guide/view/index/html){:target="_blank"} 章节.

## 重定向

要重定向浏览器到另外的URL, 可以使用redirect()方法. 第一个参数, 指定一个*QUrl*类的实例.

```c++
 // 重定向到 www.example.org 
 redirect(QUrl("www.example.org"));
```

你可以重定向到同一台主机的其他操作(action)上.

```c++
 // 重定向到Blog控制器的index操作
 redirect( url("blog", "index") );
```

下面是另外一个有用的url方法. 它在你传递控制器名和操作名时返回相应的QUrl实例.

要重定向到同一个控制器内的其他操作,可以忽略控制名.

```c++
 // 重定向到同一个控制器的show操作
 redirect( urla("show") );
```

使用 urla()方法是非常重要的.

## 在重定向目标页显示信息

重定向指向了另外一个URL. 这是因为, 从服务器的视角看, URL接收到了一个新的目标页, 这样产生了调用另外一个控制器的操作的效果.

Treefrog框架有一种机制(通过变量)可以实现传递信息到重定向的控制器上. 见下面的例子使用tflash()或T_FLASH()方法传递变量.

```c++
 // foo - 瞬时对象(flash object)
 QString foo = "successfully";
 tflash( foo );
```

传递的对象在这里叫做"瞬时对象(flash object)".

瞬时对象(flash object)转换成重定向后的视图(view)的输出对象. 正因为如此, 它能够被echo()或者eh()方法输出显示信息.

### 使用瞬时对象(flash object)的好处
事实上, 你可以创建一个完全没有瞬时对象(flash object)的网页应用. 然而, 在遇到合适的情况下, 如果你使用它, 代码变得非常容易理解(一旦你开始用它).

我个人认为保持每个操作(action)完全对立, 减少各个操作(action)之间的依赖是比较好的也比更容易理解. 我建议不要在单个请求上调用超过一个以上的操作(action). 尽可能的保存一个请求一个操作(action)的关系.

使用瞬时对象(flash object)时, 如果为几乎相同的内容建立独立分离的显示, 代码可以变得简单些. 
*blogapp* ([教程](/ch/user-guide/tutorial/index/html){:target="_blank"})使用*create* 和 *show* 操作(action)的方式就是一个好的例子. 虽然这些操作(action)的处理是不一样的, 但都是仅仅显示一条blog的处理结果. 在create操作, 成功完成录入后, 这个结果将显示在重定向的show操作上. 同时, 这个信息"Created successfully." 通过使用瞬时对象(flash object)显示.

事实上, 一个应用不会都是这么简单的, 因为不是经常需要获取和使用瞬时对象(flash object). 因此, 适度地使用它好了.

综上所述, 在实践中大部分的操作(action)都归结于使用*redirect()*或者*render()*方法.

**顺便说一下** <br>
按照上面所说的方式, 但一个重定向发生时, 一旦访问到达服务器时处理就被中断了(服务器返回新URL, 客户端按新URL再次发出请求). 鉴于瞬时对象(flash object)是通过会话的方式实现的, 存活下来的对象可以在其他操作(action)环境下使用.

## staticInitialize() 方法

当启动后, 程序只有一个进程. 你可能希望提前从数据库读取一些初始化信息.

这种情况下, 将处理过程写在*ApplicationController#staticInitialize()*内.

```c++
 void ApplicationController::staticInitialize()
 {
  // 处理过程..
 }
```

当服务器进程启动后, staticInitialize()方法只会调用一次.

## 控制器(controller)实例的生命周期

控制器(controller)的实例在被调用前创建, 当操作(action)执行完后被销毁. 这意味着控制器的创建和销毁是基于每个HTTP请求的.<br>
这样规定后, 控制器(controller)通常没有实例变量. 请按同样的方式实现*ApplicationController*.