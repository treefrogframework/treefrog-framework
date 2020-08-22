---
title: URL路由
page_id: "050.030"
---

## URL路由

URL路由是一种决定请求的URL如何调用操作(action)的一种机制. 从浏览器收到一个请求后, 将使用路由规则进行URL检查, 如果匹配规则, 根据规则定义的操作(action)将被调用. 如果没有指定操作(action), 默认的操作将会被调用.

下面是这些默认规则一点提示:

```
 /controller-name/action-name/argument1/argument2/ ...
```

让我们看看客户化的路由规则.<br>

路由规则定义写在*config/routes.cfg*文件. 对于每一个入口, 指令, 路径和操作(action)是写成单行的. 指令是指选择*match*, *get*, *post*, *put* 或者 *delete*.

此外, 以'#'开头的行表示注释行.

这里有个例子:

```
 match  /index  Merge.index
```

在这个例子中, 如果浏览器请求'/index', 不论是POST请求还是GET请求, 控制器将返回*Merge*控制器的*index*操作(action). 
接下来的例子定义了get指令:

```
 get  /index  Merge.index
```

在这个例子中, 路由规则仅在使用GET请求时才起作用. 如果是POST请求, 将会被拒绝.

类似地, 如果定义了一个Post指令, 仅对POST请求有效. GET请求将会被拒绝.

```
 post /index  Merge.index
```

下面的内容是关于如何传递参数给操作(action). 假设已经定义了下面的入口作为路由规则:

```
 get  /search/:params  Searcher.search
```

使用关键字':params'是非常重要的.<br>
例子*/serach/foo/*, 当使用GET请求, 一个serach操作带着参数被*Searcher*控制器调用.参数"foo"被传递.
类似地, /serch/foo/bar, 一个操作(action)带着两个参数("foo" 和"bar")将被调用

```
 /search/foo     ->   调用SearcherController的search("foo")
 /search/foo/bar ->   调用SearcherController的search("foo", "bar") of 
```

## 显示路由

构建应用程序后，以下命令将显示当前的路由信息​​。
```
 $ treefrog --show-routes
 Available controllers:
   match   /blog/index  ->  blogcontroller.index()
   match   /blog/show/:param  ->  blogcontroller.show(id)
   match   /blog/create  ->  blogcontroller.create()
   match   /blog/save/:param  ->  blogcontroller.save(id)
   match   /blog/remove/:param  ->  blogcontroller.remove(id)
```
