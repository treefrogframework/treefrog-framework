---
title: 静态内容
page_id: "070.030"
---

## 静态内容

将浏览器可以访问的静态内容放在*public*目录内. 在这里仅放置公共文件.

例如, 假设一个HTML文件这样存放*public/sample.html*. 当你从浏览器访问*http://URL:PORT/sample.html*, 在应用服务器(AP server)运行的情况下, 它的内容将可以显示.

在使用生成器命令行生成应用程序的骨架后, 下面的子目录将被创建.

<div class="table-div" markdown="1">

| 目录Directory      | 文件类型File Type     | URL路径path |
|--------------------|-----------------------|-------------|
| public/images/     | 图像文件              | /images/... |
| public/js/         | 脚本                  | /js/...     |
| public/css/        | 样式表                | /css/...    |

</div><br>

你可以在public目录下自由的创建子目录.

## 互联网媒体类型(MIME type)

当网页服务器返回静态内容时, 这个规则是在反馈的content-type 头字段中设置MIME type. 它们是这样的字符串, 例如"text/html"和"image/jpg". 有了这些信息, 浏览器就能判定发送过来的数据的格式.

Treefrog框架通过使用文件扩展名返回互联网媒体类型, 参考定义在*config/initializers/internet_media_types.ini*文件中. 在那个文件中, 文件的扩展和互联网媒体类型用"="连接起来, 一行一行地进行定义. 就像下面的表:

```
pdf=application/pdf
js=application/javascript
zip=application/zip
 :
```

如果这些互联网媒体类型不能覆盖你的需求, 你可以在文件总增加其他类型. 做完这些后, 你应重启应用服务器来反射你已添加的定义信息.

## 错误显示

应用服务器总是需要返回一些响应, 即使发生了一些错误或者异常. 这些情况下, 错误响应的状态代码在中[RFC](http://www.ietf.org/rfc/rfc2616.txt){:target="_blank"}已经定义.<br>
在这个框架中, 当发生错误或异常时, 下面文件的内容将作为响应返回.

<div class="table-div" markdown="1">

| 情况Cause                             | 静态文件Static File     |
|---------------------------------------|-------------------------|
| 未找到Not Found                       | public/404.html         |
| 请求实体太大Request Entity Too Large  | public/413.html         |
| 内部服务器错误Internal Server Error   | public/500.html         |

</div><br>

通过修改这些静态文件, 你可以改变显示的内容.

通过调用下面的操作(action), 你将能够返回静态文件来显示错误.这样, 当401设置成响应的状态代码时, *public/401.html*的内容将被返回.

```c++
renderErrorResponse(401);
```

还有, 在浏览器中显示错误页面的还有一个方法, 使用重定向到给定的URL.

```c++
redirect(QUrl("/401.html"));
```

## 发送文件

如果你像从服务器发送文件, 使用sendFile()方法.定义文件路径为第一个参数, 定义内容类型为第二个参数. 发送的文件不需要存放在*public*目录中.

```c++
sendFile("filepath.jpg", "image/jpeg");
```

如果你在这函数中发送一个文件, 在浏览器端将执行一个文件下载的处理. 一个对话框将显示, 询问用户是打开还是保存文件. 这个函数像HTTP响应一样发送文件, 和render()方法是一样的处理过程. 因此, 控制器(controller)不再用render()方法输出模版.

顺便说一句, 这里的文件路径, 如果你定义的是一个绝对路径, 应该确保它可以被找到.如 果你使用Tf::app->webRootPath()函数, 你将获得应用程序的绝对路径, 所以你可以很容易创建一个文件的绝对路径.要使用这样函数, 请在头文件中包含TwebAplication

```c++
#include <TWebApplication>
``` 

## 发送数据

要发送内存中的数据, 而不是一个文件, 你可以使用sendData()方法:

```c++
QByteArray data;
data = ...
sendFile(data, "text/plain");
``` 

相比与sendFile()方法, 你可以省略访问的处理减少开销.<br>
有点类似于文件下载的操作在浏览器端被执行, 在那之后不能调用render()方法(如果这样做,可能会不工作).