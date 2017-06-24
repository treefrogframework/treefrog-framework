---
title: Cookie
page_id: "050.020"
---

## Cookie

Cookies是存储在浏览器的信息, 包含了用于浏览器和在线网页应用之间共享的键值对. 有一些基本的限制, 但是cookies的功能几乎所有的浏览器都支持. 如前面的章节中描述的, 对于建立会话来说,Cookies是必须的.

Cookies系统源于网景通信公司(Netscape Communications Corporation)的建议, 在变成标准前以RFC发布.

## 保存字符串到Cookie

Cookies由用户访问的网页应用生成. 根据Cookies的规范, 它们应该保存为键值对用于HTTP响应.

当相关的操作(action)发生时, 下面的函数被调用,键值对将保存在cookie内.

```c++
addCookie("key1", "Hello world.");
```

Cookie可以设置失效日期. Cookie是附在HTTP请求上的, 在服务器和浏览器之间发送.一旦到达失效日期, Cookie会被自动的从浏览器擦除并最终消失. 失效日期被定义成addCookie()函数的第三个参数. 更多信息请参见[API参考](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}.

## 从Cookie读取字符串

通过使用键, 按下面的方式可以从HTTP请求的Cookies中获得的键对应的值.

```c++
QByteArray text = httpRequest().cookie("key1");
// text = "Hello world."
```