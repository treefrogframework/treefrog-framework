---
title: WebSocket
page_id: "090.0"
---

## WebSocket

WebSocket はサーバとクライアントとで双方向通信を行う通信規格であり、主要なブラウザは実装が完了しています。

HTTP では、基本的にリクエスト毎に接続・切断を繰り返します。長い時間コネクションを張ることは想定されていません。
一方WebSocketでは、一度TCPコネクションが成立したら切らずに張り続けます。接続中はどちらからメッセージを送信しても構いません。長時間接続することを想定しているので、相手の生死を確認する Ping/Pong フレームが定義されていたりします。

また、コネクションを切断しないことからステートフルになるので、クッキーでセッションIDを返すことをしなくとも良いということです。

##### WebSocket はステートフルで双方向通信

クライアントがブラウザの場合には、WebSocket の接続を確立したページを閉じてしまったり、別のページに移動してしまうと、その接続は切れてしまいます。

ブラウザでの WebSocket は JavaScript で実装するので、そのコンテキスト（オブジェクト）がなくなればコネクションが切れるのは当然と言えば当然です。切断のタイミングがページ遷移時であることを考えると、実際には長時間接続しつづけるケースはあまり多くないと考えられます。

さて、WebSocket に対応したブラウザを用意し、JavaScript を書いてみます。

まず、WebSocketオブジェクト生成します。パラメータには、ws:// または wss:// から始まるURLを渡します。
生成のタイミングで接続処理が行われます。

WebSocket オブジェクトに対し、次のイベントハンドラを登録できます。

<div class="table-div" markdown="1">

| 名称      	| 説明           	|
|-----------|-------------------|
| onopen    | 開始ハンドラ   	|
| onclose   | 切断ハンドラ   	|
| onmessage | 受信ハンドラ   	|
| onerror   | エラーハンドラ 	|

</div>

WebSocket オブジェクトのメソッドは次のとおり。

<div class="table-div" markdown="1">

| メソッド    	| 説明          	 |
|---------------|----------------|
| send(msg)   	| メッセージ送信  |
| close(code) 	| 切断           |

</div>

詳細は [http://www.w3.org/TR/websockets/](http://www.w3.org/TR/websockets/){:target="blank"} をご覧ください。

## チャットアプリを作る

これらを踏まえ、チャットアプリを作ってみましょう。
名前とメッセージを書き込むと、ページを開いている全員に配信されるようにします。

まずはクライアントサイドから作成しましょう。以下の例では[jQuery](https://jquery.com/){:target="_blank"}を使っています。

**HTML （抜粋）**
public/index.html として保存します。

```
<!-- メッセージ表示領域 -->
<div id="log" style="max-width: 900px; max-height: 480px; overflow: auto;"></div>
<!-- 入力領域 -->
Name  <input type="text" id="name" />
<input type="button" value="Write" onclick="sendMessage()" />
<textarea id="msg" rows="4"></textarea>
```

次にJavaScriptの例です。

```js
$(function(){
    // create WebSocket to 'chat' entpoint
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
// メッセージ送信
// 名前、時刻、メッセージを１つにして送信
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

次にサーバサイドを作ります。
足場を作ります。

```
 $ tspawn new chatapp
  created   chatapp
  created   chatapp/controllers
  created   chatapp/models
   :
```

WebSocketのやりとりを行うエンドポイント（端点）を作成します。
JavaScript の WebSocketオブジェクト生成時に渡したURLのパスに設定した名前（この例では'chat'）で作ります。そうしないと動作しません！

```
 $ cd chatapp
 $ tspawn websocket chat
  created   controllers/applicationendpoint.h
  updated   controllers/controllers.pro
  created   controllers/applicationendpoint.cpp
    :
```

生成された chatendpoint.h は以下のとおりです。
特に修正する必要はないでしょう。

```c++
class T_CONTROLLER_EXPORT ChatEndpoint : public ApplicationEndpoint
{
public:
    ChatEndpoint() { }
    ChatEndpoint(const ChatEndpoint &other);
protected:
    bool onOpen(const TSession &httpSession);        // 開始ハンドラ
    void onClose(int closeCode);                     // 終了ハンドラ
    void onTextReceived(const QString &text);        // テキスト受信ハンドラ
    void onBinaryReceived(const QByteArray &binary); // バイナリ受信ハンドラ
};
```

**onOpen() ハンドラについて説明：**
引数である httpSession はその時点のHTTPのセッションオブジェクトが渡されます。
エンドポイントでは、これは読み取り専用であり、内容を変更できません （将来的には対応するかも）。

代わりに、WebSocketSession オブジェクトを使って、ここに情報を保存しましょう。 エンドポイントクラスの各メソッド内において、session()メソッドで取り出せます。 ちなみに、その情報はメモリ上に置かれるので、大きなサイズのものは保存するとコネクションが増えるとメモリを圧迫します。

また、onOpen() の戻り値で false を返すと、WebSocketの接続を拒否することができます。接続リクエストを全て受け入れたくない場合、例えばHTTPセッションに秘密の値を保存しておき、その値が正しい場合のみ受け入れるという実装が可能です。

次に chatendpoint.cpp です。
受信したテキストを参加者全員に送信したいわけですが、出版/購読型(Pub/Sub)方式で行います。

まず、受信者はある"トピック"を購読するよう登録します。送信者はその"トピック"へメッセージを送信すると、購読者全員に配信されます。

コードは以下のようになります

```c++
#define TOPIC_NAME "foo"
ChatEndpoint::ChatEndpoint(const ChatEndpoint &)
    : ApplicationEndpoint()
{ }

bool ChatEndpoint::onOpen(const TSession &)
{
    subscribe(TOPIC_NAME);  // 購読を開始する
    publish(TOPIC_NAME, QString(" [ New person joined ]\n"));
    return true;
}

void ChatEndpoint::onClose(int)
{
    unsubscribe(TOPIC_NAME);  // 購読を停止する
    publish(TOPIC_NAME, QString(" [ A person left ]\n"));
}

void ChatEndpoint::onTextReceived(const QString &text)
{
    publish(TOPIC_NAME, text);  // メッセージを配信する
}

void ChatEndpoint::onBinaryReceived(const QByteArray &)
{ }
```

### ビルド

今回のケースでは、**VIEW**は使わないので、ビルドから外します。
*chatapp.pro* を次のように編集して保存します。

```
 TEMPLATE = subdirs
 CONFIG += ordered
 SUBDIRS = helpers models controllers
```

ビルドコマンド：

```
 $ qmake -r
 $ make   (Windowsの場合は nmake)
 $ treefrog -d
 (停止コマンド)
 $ treefrog -k stop
```

ブラウザを起動して、http://(host):8800/index.html にアクセスしてみましょう。

ちゃんと動きましたか。

実装したものを公開していますので、参考になさってください。
[http://chatsample.treefrogframework.org/](http://chatsample.treefrogframework.org/){:target="_blank"}

上記サンプルから、次の機能を追加しています。

* 直近のメッセージ30件をDBに保存している。
* そのメッセージを接続直後(onOpen())に送信している。
* CSS を追加して見栄えをよくしている。

## キープアライブ

TCPセッションにおいて無通信状態が長く続くと、ルータなどの通信機器はその通信路を解放（ルーティングを止めて）してしまいます。こうなると、サーバからメッセージを送信しても、クライアントにはもう届きません。

これを回避するために、定期的に通信を行って通信路を確保しつづける必要があります。WebSocketにおけるキープアライブは、Ping/Pong フレームを送受信することで実現しています。

TreeFrog では、 エンドポイントクラスの keepAliveInterval() の戻り値で設定します。単位は秒です。

```c++
int keepAliveInterval() const { return 300; }
```

値が 0 の場合は、キープアライブ機能は作動しません。デフォルトは 0 （キープアライブしない）です。

キープアライブを行うことで、通信路が有効かどうかを確認するだけでなく、ホストのソフトウェアがダウンしていないことも確認することができるわけですが、それを検知するAPIは現時点(2015/6)で TreeFrog には実装されていません。


**参考**
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
