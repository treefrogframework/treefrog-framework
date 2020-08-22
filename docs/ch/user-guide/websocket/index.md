---
title: WebSocket
page_id: "090.0"
---

## WebSocket

WebSockets是一种通信标准, 它支持服务器和客户端的双向通信, 被浏览器广泛支持.

HTTP的每次请求都要建立连接和断开连接, 不支持长时间的连接.<br>
另一方面, 在TCP连接成功建立后WebSocket保持连接. 在持续连接期间, 信息可以从服务器端和客户端发送. 因为它假设连接是持续的, 叫做*乒乓Ping/Pong*的数据帧用来确认对方的生存和死亡.

此外, 因为连接是有状态的(维持一个建立的连接), 你不需要使用cookie返回会话的ID.

##### 概要:WebSocket是一个有状态的双向通信

客户端是浏览器时, 如果跳到另外一个页面或者页面上建立的WebSocket连接关闭时, 连接会丢失.

因为WebSocket在浏览器端是用JavaScript实现的, 本质上,如果WebSocket的环境(对象)消失了, WebSocket连接将会丢失, 因此可以这样认为连接的时间和页面的切换是相联系的. 在实践中认为不会有很多实际上长期的连接的例子.

现在, 让我们准备一个浏览器兼容的WebSocket, 然写一些脚本.

首先, 我们将生成一个WebSocket对象. 传递*ws://*或者*wss://*起头的URL作为参数.<br>
连接的处理在创建的时刻进行.

可以为WebSocket对象注册下面的事件处理器(event handers):

<div class="table-div" markdown="1">

| 名称      | 描述           |
|-----------|----------------|
| onopen    | open处理器     |
| onclose   | close处理器    |
| onmessage | message处理器  |
| onerror   | error 处理器   |

</div><br>

下面是WebSocket对象的方法:

<div class="table-div" markdown="1">

| 方法          | 描述         |
|---------------|--------------|
| send(msg)     | 发送消息     |
| close(code)   | 断开连接     |

</div><br>

更多详细信息,请看 [http://www.w3.org/TR/websockets/](http://www.w3.org/TR/websockets/){:target="_blank"}.

## 创建一个聊天程序

让我们基于这些处理器(handlers)生成一个聊天程序.<br>
这个程序的主要功能就是: 用户可以输入他的名字和一个i奥信息到一个非常基本的HTML表单. 在用户点击"send"按键后, 应用将发送数据到服务器并将它保存在数据库中. 任何程序的访问者访问网页时, 举例, 最近30条信息将发送给访问者并显示在聊天窗口中.

我们将说一下如何将信息发送给*订阅sbuscribed*了特定*主题(topic)*的用户.

现在, 让我们开始实现它们, 先从客户端开始.**说明:** 下面的例子使用了 [jQuery](https://jquery.com/){:target="_blank"}.

**HTML (节选)**<br>
保存为public/index.html.

```
<!-- 消息显示区域 -->
<div id="log" style="max-width: 900px; max-height: 480px; overflow: auto;"></div>
<!-- 输入区域 -->
Name  <input type="text" id="name" />
<input type="button" value="Write" onclick="sendMessage()" /><br>
<textarea id="msg" rows="4"></textarea>
```

这里是JavaScript的代码.

```js
$(function(){
    // 创建'cat'端点的WebSocket
    ws = new WebSocket("ws://" + location.host + "/chat");
 
    // 接收到信息
    ws.onmessage = function(message){
        var msg = escapeHtml(message.data);
        $("#log").append("<p>" + msg + "</p>");
    }
 
    // 错误事件
    ws.onerror = function(){
        $("#log").append("[ Error occurred. Try reloading. ]");
    }
 
    // 断开事件
    ws.onclose = function(){
        $("#log").append("[ Connection closed. Try reloading. ]");
    }
});
// 发送包含 '名字Name' 和 '时间Time'的信息 
function sendMessage() {
    if ($('#msg').val() != '') {
        var name = $.trim($('#name').val());
        if (name == '') {
            name = "(I'm John Doe)";
        }
        name += ' : ' + (new Date()).toISOString() + '\n';
        ws.send(name + $('#msg').val());
        $('#msg').val(''); // 清除信息
    }
}
```

接下来,用骨架实现服务器端.

```
$ tspawn new chatapp
  created   chatapp
  created   chatapp/controllers
  created   chatapp/models
   :
```

为WebSocket互动创建端点<br>
这里使用的名字(此例为'chat')和生成JavaScript WebSocket对象时设置URL的路径一样. 否则它不会工作.

```
$ cd chatapp
$ tspawn websocket chat
  created   controllers/applicationendpoint.h
  updated controllers/ controllers. pro
  created   controllers/applicationendpoint.cpp
    :
```

生成的*chatendpoint.h*看起来像这样.<br>
不需要进行什么特殊的修改.

```c++
class T_CONTROLLER_EXPORT ChatEndpoint : public ApplicationEndpoint
{
public:
    ChatEndpoint() { }
    ChatEndpoint(const ChatEndpoint &other);
protected:
    bool onOpen(const TSession &httpSession);        // open 处理器
    void onClose(int closeCode);                     // close 处理器
    void onTextReceived(const QString &text);        // text receive 处理器
    void onBinaryReceived(const QByteArray &binary); // binary receive 处理器
};
```

**解释 onOpen() 处理器:**<br> 
HTTP会话对象那个时刻被传递到*httpSession*参数中. 端点是只读的并且它的内容不能被更改(我可以在将来处理它).

我们将使用*WebSocketSession*对象代替来保存信息. 在endpoint类的每个方法中, 可以使用*session()*方法获取信息.顺便说一下, 因为信息是保存在内存里的, 如果数据量大, 在连接负载增加时内存会被压缩.

还有, 如果*onOpen()*返回*false*, WebSocket连接会被拒绝. 如果不想接受所有的连接请求, 可以实现一些分类的秘密的值存储在HTTP会话中. 例如, 仅在它们正确的情况下才接受.

接下来是*chatendpoint.cpp*.<br>
我们希望发送收到的文字给所有的订阅者. 这个可以使用*publication/subscription*(Pub/Sub)方法.

首先, 接受方需要订阅某个"主题(tipic)". 当某人发送信息给那个"主题(tipic)"时, 那条消息将会发送给所有的订阅者.

这种行为的代码看起来像这样:

```c++
#define TOPIC_NAME "foo"
ChatEndpoint::ChatEndpoint(const ChatEndpoint &) : ApplicationEndpoint()
{ }

bool ChatEndpoint::onOpen(const TSession &)
{
    subscribe(TOPIC_NAME);  // 开始订阅
    publish(TOPIC_NAME, QString(" [ New person joined ]\n"));
    return true;
}

void ChatEndpoint::onClose(int)
{
    unsubscribe(TOPIC_NAME);  // 停止订阅
    publish(TOPIC_NAME, QString(" [ A person left ]\n"));
}

void ChatEndpoint::onTextReceived(const QString &text)
{
    publish(TOPIC_NAME, text);  // 发送信息
}

void ChatEndpoint::onBinaryReceived(const QByteArray &)
{ }
```

### 构建

此例中, 我将不使用**VIEW**, 所以我将会从构建中删掉它.
编辑*chatapp.pro*如下并保存它.

```
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = helpers models controllers
``` 

构建命令:

```
$ qmake -r
$ make   (nmake on Windows)
$ treefrog -d
 (停止命令)
$ treefrog- k stop
```

让我们启动浏览器并访问*http://(host):8800/index.html*.

它正常工作了吗?

我们现在将我们刚才实现的发布, 所以这个结果应该看起来像这样[http://chatsample.treefrogframework.org/](http://chatsample.treefrogframework.org/){:target="_blank"}

下面的这些功能已经添加到上面的例子中:

* 数据库中最近的30条信息.
* 在连接(*onOpen*)后信息立即被发送
* 增加了一些样式表使应用看起来漂亮一些

## 保持在线(keep alive)

当TCP会话长时间持续在无通讯状态, 通信设备例如路由器, 将会停止路由. 这种情况下, 即使你从服务器发送一个消息, 它再也到达不了客户端了.

要避免这种情况, 需要通过定期通信来保持在线(keep alive). *保持在线(keep alive)在WebSocket中是通过发送和接收*乒乓Ping/Pong*数据帧实现的.

在Treefrog中, 它是通过endpoint类的*keepAliveInterval()*的返回值设置的. 时间单位是秒.

```c++
int keepAliveInterval() const { return 300; }
```

如果值为0, 保持在线功能将不会工作.默认值是0(不工作).

通过保存在线, 你不仅可以检查是否通信线路是通畅的, 还可以检查主机软件是否已经关闭. 然而, Treefrog目前(2015/6)还没有实现检测当前是否可用的API.

**参考**<br>
tspawn HELP

```
 $ tspawn -h
 usage: tspawn <subcommand> [args]
 Type 'tspawn --show-drivers' to show all the available database drivers for Qt.
 Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.
 Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.
 Type 'tspawn --show-collections' to show all collections in the MongoDB.

 Available subcommands:
   new (n)         <application-name>
   scaffold (s)    <table-name> [model-name]
   controller (c)  <controller-name> action [action ...]
   model (m)       <table-name> [model-name]
   usermodel (u)   <table-name> [username password [model-name]]
   sqlobject (o)   <table-name> [model-name]
   mongoscaffold (ms) <model-name>
   mongomodel (mm) <model-name>
   websocket (w)   <endpoint-name>
   validator (v)   <name>
   mailer (l)      <mailer-name> action [action ...]
   delete (d)      <table-name or validator-name>
```