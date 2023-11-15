---
title: WebSocket
page_id: "090.0"
---

## WebSocket

WebSocket is a communication standard that performs bidirectional communication between server and client, and is widely supported by browsers.

HTTP repeatedly establishes connections and disconnections for every request that is made and it is not supposed to establish connections for a long time.<br>
WebSocket, on the other hand, maintains the TCP connection once it has been established successfully. During a persisting connection, messages can be sent from either side. Because it is assumed that a connection will be long-lasting, so-called **Ping/Pong** frames are defined to confirm the other side's life and death.

Additionally, since the connection is stateful (maintaining an established connection), you don't need to return the session ID by using a cookie.

##### In short: WebSocket is a stateful and bidirectional communication

In case the client is a browser, the connection will be lost, if you move to another page or if the WebSocket connection established for a page is closed.

Since WebSocket in the browser is implemented with JavaScript, it is natural that the Websocket connection will be lost if its context (object) disappears, therefore it can be considered that the timing of the disconnect is associated with the page transition. In practice it is considered that there are not many cases in which a connection is actually long-lasting.

Now, let's try to prepare a WebSocket that is compatible with the browser and write some JavaScript.

First, we will generate a WebSocket object. As for the parameters, pass in the URL starting with *ws://* or *wss://*. <br>
The connection processing is performed at the time of creation.

The following event handlers can be registered for the WebSocket object:

<div class="table-div" markdown="1">

| Name      	| Description  |
|-----------|----------------|
| onopen    	| open handler |
| onclose   	| close handler  |
| onmessage 	| message handler |
| onerror   	| error handler |

</div><br>

The methods of the WebSocket object are as follows:

<div class="table-div" markdown="1">

| Method    	| Description |
|---------------|--------------|
| send(msg)   	| send message |
| close(code) 	| disconnect |

</div><br>

For more insight details, please visit [http://www.w3.org/TR/websockets/](http://www.w3.org/TR/websockets/){:target="_blank"}.

## Make a chat application

Let's make a chat application based on these handlers. <br>
The main function of this application is as follow: the user can enter his name and a message into a very basic HTML form. After the user has clicked the "send" button, the application will send the data to a server and stores it inside a database. Whenever any visitor of this application accesses the Web page, for example, the most recent 30 messages will be delivered to the visitor and displayed in the chat window.

We will also talk about how to send messages only to persons who have *subscribed* to a specific *topic*.

Now let's start with the implementation, starting with the client side. **Note:** the following example uses [jQuery](https://jquery.com/){:target="_blank"}.

**HTML (excerpt)**<br>
Save it as public/index.html.

```
<!-- Message display area -->
<div id="log" style="max-width: 900px; max-height: 480px; overflow: auto;"></div>
<!-- Input area -->
Name  <input type="text" id="name" />
<input type="button" value="Write" onclick="sendMessage()" /><br>
<textarea id="msg" rows="4"></textarea>
```

Here is an example made with JavaScript.

```js
$(function(){
    // create WebSocket to 'chat' endpoint
    ws = new WebSocket("ws://" + location.host + "/chat");

    // message received
    ws.onmessage = function(message){
        var msg = escapeHtml(message.data);
        $("#log").append("<p>" + msg + "</p>");
    }

    // error event
    ws.onerror = function(){
        $("#log").append("[ Error occurred. Try reloading. ]");
    }

    // onclose event
    ws.onclose = function(){
        $("#log").append("[ Connection closed. Try reloading. ]");
    }
});
// Sending as one message containing 'Name' and 'Time'
function sendMessage() {
    if ($('#msg').val() != '') {
        var name = $.trim($('#name').val());
        if (name == '') {
            name = "(I'm John Doe)";
        }
        name += ' : ' + (new Date()).toISOString() + '\n';
        ws.send(name + $('#msg').val());
        $('#msg').val(''); // clear
    }
}
```

Next, let's implement the server side by using scaffold.

```
 $ tspawn new chatapp
  created   chatapp
  created   chatapp/controllers
  created   chatapp/models
   :
```

Create an endpoint for the WebSocket interaction. <br>
This is made with the name ('chat' in this example) set as the path of URL passed when generating the JavaScript WebSocket object. Otherwise it will not work!

```
 $ cd chatapp
 $ tspawn websocket chat
  created   controllers/applicationendpoint.h
  updated   controllers/controllers.pro
  created   controllers/applicationendpoint.cpp
    :
```

The generated *chatendpoint.h* looks then as follows.<br>
There is no need to modify it in particular.

```c++
class T_CONTROLLER_EXPORT ChatEndpoint : public ApplicationEndpoint
{
public:
    ChatEndpoint() { }
    ChatEndpoint(const ChatEndpoint &other);
protected:
    bool onOpen(const TSession &httpSession);        // open handler
    void onClose(int closeCode);                     // close handler
    void onTextReceived(const QString &text);        // text receive handler
    void onBinaryReceived(const QByteArray &binary); // binary receive handler
};
```

**Explaining the onOpen() handler:**<br>
The HTTP session object at that time is passed in the *httpSession* argument. The endpoint is read only and its content cannot be changed (I may deal with this in the future).

Instead, let's save the information here using the *WebSocketSession* object. Within each method of the endpoint class, the information can be retrieved using the *session()* method. By the way, since the information is stored in the memory, if you store data of a large size, memory will be compressed if the connection load increases.

Furthermore, the WebSocket connection can be rejected if the return value of *onOpen()* is *false*. If you don't want to accept all connection requests, it is possible to implement some sort secret values stored in HTTP sessions, for example, accepting them only if they are correct.

Next is the *chatendpoint.cpp*. <br>
We want to send the received text to all the subscribers. This can be done by using the *publication/subscription* (Pub/Sub) method.

First, the recipient needs to subscribe to a certain "topic". When someone is sending a message to that "topic", that message will delivered to all its subscribers.

The code for this behaviour looks like this:

```c++
#define TOPIC_NAME "foo"
ChatEndpoint::ChatEndpoint(const ChatEndpoint &)
    : ApplicationEndpoint()
{ }

bool ChatEndpoint::onOpen(const TSession &)
{
    subscribe(TOPIC_NAME);  // Start subscription
    publish(TOPIC_NAME, QString(" [ New person joined ]\n"));
    return true;
}

void ChatEndpoint::onClose(int)
{
    unsubscribe(TOPIC_NAME);  // Stop subscription
    publish(TOPIC_NAME, QString(" [ A person left ]\n"));
}

void ChatEndpoint::onTextReceived(const QString &text)
{
    publish(TOPIC_NAME, text);  // Send message
}

void ChatEndpoint::onBinaryReceived(const QByteArray &)
{ }
```

### Build

In this case, I will not use **VIEW**, so I will remove it from the build.
Edit *chatapp.pro* as follows and save it.

```
 TEMPLATE = subdirs
 CONFIG += ordered
 SUBDIRS = helpers models controllers
```

Build command:

```
 $ qmake -r
 $ make   (nmake on Windows)
 $ treefrog -d

 (stop command)
 $ treefrog -k stop
```

Let's start the browser and access *http://(host):8800/index.html*.

Did it work properly?

We are now publishing what we just have implemented, so the result should look like the following: [http://chatsample.treefrogframework.org/](http://chatsample.treefrogframework.org/){:target="_blank"}

The following functions are added from the sample above:

* 30 most recent messages are stored in DB
* the message is sent immediately after connection (*onOpen()*)
* adding some stylish CSS makes your application look good

## Keep alive

When the non-communication state lasts for a long time in a TCP session, communication devices, such as routers, will stop routing. In this case, even if you send a message from the server, it will not reach the client anymore.

In order to avoid this, it is necessary to keep connection alive by periodically communicating. *Keep alive* in WebSocket is achieved by sending and receiving *Ping/Pong* frames.

In TreeFrog, it is set by the return value of the *keepAliveInterval()* of the endpoint class. The time unit is in seconds.

```c++
int keepAliveInterval() const { return 300; }
```

If the value is 0, the keep-alive function will not work. The default is 0 (not keepalive).

By keeping alive, you can check not only whether the communication path is valid, but also you can check that the host software is not down. However, the API that detects it is currently available (2015/6), but not implemented in TreeFrog yet.

**Reference**<br>
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