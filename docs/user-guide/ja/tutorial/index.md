---
title: チュートリアル
page_id: "030.0"
---

## チュートリアル

TreeFrog アプリケーションの作成をひとめぐりしてみましょう。<br>
リスト表示、テキストの追加／編集／削除ができる単純なブログシステムをつくってみます。

## アプリケーションのスケルトンを生成

blogapp という名でスケルトン（ディレクトリツリーと各種設定ファイル）を作ってみます。コマンドライン から次を実行します。Windows の場合は、TreeFrog Command Prompt 上で実行してください。

```
 $ tspawn new blogapp
  created   blogapp
  created   blogapp/controllers
  created   blogapp/models
  created   blogapp/models/sqlobjects
  created   blogapp/views
  created   blogapp/views/layouts
  created   blogapp/views/mailer
  created   blogapp/views/partial
   :
```

## テーブルを作成

データベースにテーブルを作ります。タイトルと内容（ボディ）のフィールドを作りましょう。<br>
ここでは、MySQL と SQLite の例を示します。

**MySQL の例：**<br>
文字セットは UTF-8 に設定します。データベースの設定ファイルでその指定するか（正しく設定されたか確認しましょう。[FAQ 参照](http://www.treefrogframework.org/ja/%E3%83%89%E3%82%AD%E3%83%A5%E3%83%A1%E3%83%B3%E3%83%88/faq){:target="_target"}）、下記のようにデータベースを生成する際に指定することもできます。また、mysql コマンドラインツールにパスを通しておいてください。

```
 $ mysql -u root -p
 Enter password:

 mysql> CREATE DATABASE blogdb DEFAULT CHARACTER SET utf8;
 Query OK, 1 row affected (0.01 sec)

 mysql> USE blogdb;
 Database changed

 mysql> CREATE TABLE blog (id INTEGER AUTO_INCREMENT PRIMARY KEY, title VARCHAR(20), body VARCHAR(200), created_at DATETIME, updated_at DATETIME, lock_revision INTEGER) DEFAULT CHARSET=utf8;

 Query OK, 0 rows affected (0.02 sec)

 mysql> DESC blog;
 +---------------+--------------+------+-----+---------+----------------+
 | Field         | Type         | Null | Key | Default | Extra          |
 +---------------+--------------+------+-----+---------+----------------+
 | id            | int(11)      | NO   | PRI | NULL    | auto_increment |
 | title         | varchar(20)  | YES  |     | NULL    |                |
 | body          | varchar(200) | YES  |     | NULL    |                |
 | created_at    | datetime     | YES  |     | NULL    |                |
 | updated_at    | datetime     | YES  |     | NULL    |                |
 | lock_revision | int(11)      | YES  |     | NULL    |                |
 +---------------+--------------+------+-----+---------+----------------+
 6 rows in set (0.01 sec)

 mysql> quit
 Bye
```

**SQLite の例：**<br>
データベースファイルはdb ディレクトリに置くことにします。

```
 $ cd blogapp
 $ sqlite3 db/blogdb
 SQLite version 3.6.12
 sqlite> CREATE TABLE blog (id INTEGER PRIMARY KEY AUTOINCREMENT, title VARCHAR(20), body VARCHAR(200), created_at TIMESTAMP, updated_at TIMESTAMP, lock_revision INTEGER);
 sqlite> .quit
```

これで id、title, bady, created_at, updated_at, lock_revision のフィールドを持つ blog テーブルができました。

created_at と updated_at のフィールドがあると、TreeFrog はそれぞれ作成日時、更新日時を自動で挿入・更新してくれます。lock_revision フィールドは、楽観的ロックを実現するためのもので、integer 型で作っておきます。

### 楽観的ロック (Optimistic Lock)

楽観的ロックとは、更新の時に行ロックをかけず、他から更新されていないことを検証しつつデータを保存することです。実際のロックはかけないので、処理速度の向上がちょっとだけ期待できます。詳細は[O/Rマッピングの章]({{ site.baseurl }}/user-guide/ja/model/or-mapping.html){:target="_blank"}をご覧ください。

## データベースの情報を設定

データベースの情報を config/database.ini ファイルに設定します。<br>
エディタでファイルを開き、[dev] の各項目に環境に応じた適切な値を入力して、保存します。

MySQL の例：

```
 [dev]
 DriverType=QMYSQL
 DatabaseName=blogdb
 HostName=
 Port=
 UserName=root
 Password=root
 ConnectOptions=
```

SQLite の例：

```
 [dev]
 DriverType=QSQLITE
 DatabaseName=db/blogdb
 HostName=
 Port=
 UserName=
 Password=
 ConnectOptions=
```

正しく設定されたか、DBにアクセスしてテーブルを表示してみましょう。

```
 $ cd blogapp
 $ tspawn --show-tables
 DriverType:   QSQLITE
 DatabaseName: db\blogdb
 HostName:
 Database opened successfully
 -----
 Available tables:
   blog
```

このように表示されれば成功です。

もし 使用する SQL ドライバがQt SDK に組み込まれていないと、ここでエラーが発生します。

```
 QSqlDatabase: QMYSQL driver not loaded
```

この場合、[ダウンロードのページ](http://www.treefrogframework.org/ja/%E3%83%80%E3%82%A6%E3%83%B3%E3%83%AD%E3%83%BC%E3%83%89){:target="_blank"}でSQLデータベースドライバをダウンロードして、インストールしてください。<br>
組み込まれたSQLドライバは次のコマンドで確認することができます。

```
 $ tspawn --show-drivers
 Available database drivers for Qt:
  QSQLITE
  QMYSQL3
  QMYSQL
  QODBC3
  QODBC
```

SQLite の SQL ドライバはあらかじめ組み込まれているので、ちょっと試すだけなら SQLite を使うのがいいかもしれませんね。

## テンプレートシステムの指定

TreeFrog Framework では、テンプレートシステムとして ERB と Otama のどちらかを指定します。
*development.ini* ファイルにある TemplateSystem パラメータに設定します。

```
 TemplateSystem=ERB
   または
 TemplateSystem=Otama
```

## 作ったテーブルからコードを自動生成

コマンドラインから、ジェネレータコマンド（*tspawn*）を実行し、ベースとなるコードを生成します。下記の例ではコントローラ、モデル、ビューを生成しています。引数には、テーブル名を指定します。

```
 $ tspawn scaffold blog
 DriverType: QSQLITE
 DatabaseName: db/blogdb
 HostName:
 Database open successfully
   created   controllers/blogcontroller.h
   created   controllers/blogcontroller.cpp
   updated   controllers/controllers.pro
   created   models/sqlobjects/blogobject.h
   created   models/blog.h
   created   models/blog.cpp
   updated   models/models.pro
   created   views/blog
 　 　:
```

※ tspawn の オプションによって、コントローラだけ、あるいはモデルだけ生成するように変えられます。

参考：tspawnコマンドのヘルプ
```
 $ tspawn --help
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
   validator (v)   <name>
   mailer (l)      <mailer-name> action [action ...]
   delete (d)      <table-name or validator-name>
```

## ソースコードをビルド

make する前に、一度だけ次のコマンドを実行し、Makefile を生成します。

```
 $ qmake -r "CONFIG+=debug"
```

WARNING メッセージが表示されますが、問題はありません。その後、make コマンドを実行すると、コントローラ、モデル、ビュー、ヘルパの全てをコンパイルします。

```
 $ make     （MinGWの場合は mingw32-make, MSVCの場合は nmake）
```

ビルドが成功すると、４つの共有ライブラリ（controller, model, view, helper）が lib ディレクトリに作られます。<br>
デフォルトでは、デバッグモードのライブラリが生成されますが、リリースモードのライブラリを作成するには次のコマンドでMakefileを再生成すればよいでしょう。

リリースモードのMakefile作成：

```
 $ qmake -r "CONFIG+=release"
```

## アプリケーションサーバを起動

アプリケーションのルートディレクトリに移り、アプリケーションサーバ（APサーバ）を起動します。<br>
サーバは、コマンドが実行されたディレクトリをアプリケーションルートディレクトリと見なして処理を始めます。サーバを止めるときは、Ctrl+c を押します。

```
 $ treefrog -e dev
```

Windows では、*treefrog**d**.exe* を使って起動します。

```
 > treefrogd.exe -e dev
```

Windows では、Web アプリケーションをデバッグモードでビルドした場合は *treefrog**d**.exe* を、リリースモードでビルドした場合は *treefrog.exe* を使って起動してください。<br>
##### リリースモードとデバッグモードのオブジェクトが混在すると、正常に動作しません。

バックグランドで起動する場合は、-d オプションを指定します。

```
 $ treefrog -d -e dev
```

ここまで現れた -e オプションについて補足します。<br>
このオプションの後に database.ini で設定した**セクション名**を指定することで、データベースの設定を切り替えることができます。省略時は、product を指定したとみなされます。プロジェクトの作成時は次の３つが定義されています。

<div class="table-div" markdown="1">

| セクション | 説明 |
| ------- | ------------|
| dev	  | 開発用、ジェネレータ用 |
| test	  | テスト用 |
| product |	正式版用、製品版用 |

</div>

※ -e は environment の頭文字からきています。

停止コマンド：

```
 $ treefrog -k stop
```

強制終了コマンド：

```
 $ treefrog -k abort
```

リスタートコマンド：

```
 $ treefrog -k restart
```

★ もしファイヤーウォールが設定されている場合は、ポート（デフォルト：8800）を開けてください。

## ブラウザでアクセス

ブラウザで http://localhost:8800/Blog にアクセスしてみましょう。<br>
次のような一覧画面が表示されるはずです（Railsを真似てます..）。

最初は１件も登録がありません。

<div class="img-center" markdown="1">

![Listing Blog 1]({{ site.baseurl }}/assets/images/documentation/ListingBlog-300x216.png "Listing Blog 1")

</div>

２件ほど登録してみたところ。すでに新規登録、参照、編集、削除を行うができます。<br>
日本語の表示も問題なし。とっても簡単！

<div class="img-center" markdown="1">

![Listing Blog 2]({{ site.baseurl }}/assets/images/documentation/ListingBlog2-300x216.png "Listing Blog 2")

</div>

他のフレームワークと同様に TreeFrog においても、リクエストされた URL から該当するコントローラのメソッド（アクション）を呼び出す仕組み（ルーティングシステム）が備わっています。<br>
開発したソースコードはビルドしなおせば、他のプラットフォームでも動作します。

このサンプルWebアプリケーションを公開してます。[ここにアクセスして](http://blogapp.treefrogframework.org/Blog){:target="_blank"}、遊んでみてください。デスクトップアプリケーション並の速さです。

## コントローラの中身　

生成されたコントローラの中身を見てみましょう。<br>
まずはヘッダファイル。おまじないコードが幾つかありますが、URLによるディスパッチングのために必要な措置なのです。

*public slots* の部分に、ディスパッチさせたいアクション（メソッド）を宣言するのがポイントです。そこには [CRUD](https://ja.wikipedia.org/wiki/CRUD){:target="_blank"} に相当するアクションが定義されていますね。<br>
ちなみに、*slots* キーワードは Qt  による機能拡張のものです。詳細は Qt ドキュメントをご覧ください。

```c++
class T_CONTROLLER_EXPORT BlogController : public ApplicationController
{
    Q_OBJECT
public:
    Q_INVOKABLE
    BlogController() { }
    BlogController(const BlogController &other);

public slots:
    void index();                     // 一覧表示
    void show(const QString &id);     // １件表示
    void create();                    // 新規登録
    void save(const QString &id);     // 保存（更新）
    void remove(const QString &id);   // １件削除
};

T_DECLARE_CONTROLLER(BlogController, blogcontroller)        // おまじない
```

次に、ソースファイル。<br>
ちょっと長いですが、100 行程度なのでおつきあいください。

```c++
#include "blogcontroller.h"
#include "blog.h"

BlogController::BlogController(const BlogController &)
    : ApplicationController()
{ }

void BlogController::index()
{
    auto blogList = Blog::getAll(); // Blogオブジェクトの全リストを取得
    texport(blogList);              // ビューへ値を渡す
    render();                       // ビュー（テンプレート）を描画
}

void BlogController::show(const QString &id)
{
    auto blog = Blog::get(id.toInt()); // プライマリキーでBlogモデルを取得
    texport(blog);
    render();
}

void BlogController::create()
{
    switch (httpRequest().method()) {  // httpRequestメソッドのタイプをチェック
    case Tf::Get:
        render();
        break;

    case Tf::Post: {
        auto blog = httpRequest().formItems("blog");  // 入って来るフォームデータを「blog」という変数
                                                      //（QVariantMap型）保存する
        auto model = Blog::create(blog);              // POSTされた情報からオブジェクト生成

        if (!model.isNull()) {
            QString notice = "Created successfully.";
            tflash(notice);                      // flash メッセージを設定
            redirect(urla("show", model.id()));  // show アクションへリダイレクト
        } else {
            QString error = "Failed to create."; // オブジェクト生成失敗時
            texport(error);
            texport(blog);
            render();
        }
        break;
    }

    default:
        renderErrorResponse(Tf::NotFound);       // エラーページが表示される
        break;
    }
}

void BlogController::save(const QString &pk)
{
    switch (httpRequest().method()) {
    case Tf::Get: {
        auto model = Blog::get(id.toInt()); // Get a Blog object for update
        if (!model.isNull()) {
            session().insert("blog_lockRevision", model.lockRevision()); // Sets the lock revision
            auto blog = model.toVariantMap();
            texport(blog);                  // Sends the blog-data to the view
            render();
        }
        break;
    }

    case Tf::Post: {
        QString error;
        int rev = session().value("blog_lockRevision").toInt(); // ロックリビジョンを取得
        auto model = Blog::get(id.toInt(), rev);                // データを保存するための Blog オブジェクトを取得

        if (model.isNull()) {
            error = "Original data not found. It may have been updated/removed by another transaction.";
            tflash(error);
            redirect(urla("save", id));
            break;
        }

        auto blog = httpRequest().formItems("blog");
        model.setProperties(blog);              // リクエストされたデータを設定
        if (model.save()) {                     // オブジェクトを保存（永続化）
            QString notice = "Updated successfully.";
            tflash(notice);
            redirect(urla("show", model.id())); // show アクションへリダイレクト
        } else {
            error = "Failed to update.";
            texport(error);
            texport(blog);
            render();
        }
        break;
    }

    default:
        renderErrorResponse(Tf::NotFound);
        break;
    }
}

void BlogController::remove(const QString &id)
{
    if (httpRequest().method() != Tf::Post) {
        renderErrorResponse(Tf::NotFound);
        return;
    }

    auto blog = Blog::get(id.toInt());  // Blog オブジェクトを取得
    blog.remove();                      // 削除
    redirect(urla("index"));
}

// Don't remove below this line
T_REGISTER_CONTROLLER(blogcontroller)   // おまじない
```

※ ロックリビジョンは楽観的ロックを実現するために使用されます。詳細は「モデル」の章で後述します。

ご覧のとおり、ビュー（テンプレート）に対してデータを渡すには  texport メソッドを使います。この texport メソッドの引数は QVariant のオブジェクトです。QVariant はあらゆる型になりえるので、int, QString, QList, QHash はもちろん任意のオブジェクトが渡せます。QVariant の詳細は Qt ドキュメントを参照ください。

## ビューの仕組み

TreeFrog では、今のところ２つのテンプレートシステムを採用しています。ERB と 独自システム（Otama と呼んでいます）です。Rails などで知られているとおり、ERB はHTMLにコードを埋め込むものです。

ジェネレータで自動生成されるデフォルトのビューは ERBのファイルです。index.erb の中身を見てみましょう。<br>
ご覧のように <% .. %> で囲まれた部分にC++コードを書きます。index アクションから render メソッドが呼び出されると、この index.erb の内容がレスポンスとして返されます。

```
<!DOCTYPE HTML>
<%#include "blog.h" %>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
  <title><%= controller()->name() + ": " + controller()->activeAction() %></title>
</head>
<body>
<h1>Listing Blog</h1>

<%== linkTo("New entry", urla("entry")) %><br />
<br />
<table border="1" cellpadding="5" style="border: 1px #d0d0d0 solid; border-collapse: collapse;">
  <tr>
    <th>ID</th>
    <th>Title</th>
    <th>Body</th>
  </tr>
<% tfetch(QList<Blog>, blogList); %>
<% for (const auto &i : blogList) { %>
  <tr>
    <td><%= i.id() %></td>
    <td><%= i.title() %></td>
    <td><%= i.body() %></td>
    <td>
      <%== linkTo("Show", urla("show", i.id())) %>
      <%== linkTo("Edit", urla("save", i.id())) %>
      <%== linkTo("Remove", urla("remove", i.id()), Tf::Post, "confirm('Are you sure?')") %>
    </td>
  </tr>
<% } %>
</table>
```

**次は、Otamaテンプレートシステムを見てみましょう。**

Otama はテンプレートとプレゼンテーションロジックを完全に分離したテンプレートシステムです。HTMLテンプレートには完全な HTML を記述し、動的に書き換えたい部分の要素（開始タグ）に「マーク」をつけます。プレゼンテーションロジックファイルには、その「マーク」に関連づけてロジック（C++コード）を記述します。

次の例は、テンプレートシステムに Otama を指定した時にジェネレータによって生成されるファイルです。ファイルを見ると分かりますが、HTML(バージョン5)に準拠しているので、今時のブラウザで開けばデザインは全く崩れません。

```
<!DOCTYPE HTML>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
  <title data-tf="@head_title"></title>
</head>
<body>
<h1>Listing Blog</h1>
<a href="#" data-tf="@link_to_entry">New entry</a><br />
<br />
<table border="1" cellpadding="5" style="border: 1px #d0d0d0 solid; border-collapse: collapse;">
  <tr>
    <th>ID</th>
    <th>Title</th>
    <th>Body</th>
    <th></th>
  </tr>
  <tr data-tf="@for">                  ← @for という「マーク」。以下同様。
    <td data-tf="@id"></td>
    <td data-tf="@title"></td>
    <td data-tf="@body"></td>
    <td>
      <a data-tf="@linkToShow">Show</a>
      <a data-tf="@linkToEdit">Edit</a>
      <a data-tf="@linkToRemove">Remove</a>
    </td>
  </tr>
</table>
</html>
```

「マーク」をつけるために data-tf というカスタム属性を使っています。HTML5 でいうところの Custom Data Attribute のことです。その値の"@"で始まっている文字列が「マーク」です。


次に、プレゼンテーションロジックに該当する index.otm を見てみましょう。<br>
上記のテンプレートで宣言されたマークに、ロジックが関連づけられています。マークから空行までが１セットです。ロジックの部分はほぼ C++ コードです。<br>
関連づけには演算子（~= とか :== など）も使われています。この演算子によって、振る舞いが変わります（詳細は各章で）。

```c++
#include "blog.h"    ← これは C++ コードそのまま。blog.h をインクルード
@head_title ~= controller()->controllerName() + ": " + controller()->actionName()

@for :
tfetch(QList<Blog>, blogList);    /* コントローラから渡されたデータを使う宣言 */
for (QListIterator<Blog> it(blogList); it.hasNext(); ) {
    const Blog &i = it.next();          /* i は Blog オブジェクトの参照 */
    %%        /* ループ(for文)するときのお決まりで、その要素と子要素を繰り返す */
}

@id ~= i.id()   /* @id というマークのある要素のコンテントに i.id() の結果を入れる */

@title ~= i.title()

@body ~= i.body()

@linkToShow :== linkTo("Show", urla("show", i.id()))  /* その要素と子要素を linkTo() の結果で置き換える */

@linkToEdit :== linkTo("Edit", urla("edit", i.id()))

@linkToRemove :== linkTo("Remove", urla("remove", i.id()), Tf::Post, "confirm('Are you sure?')")
```

簡単に Otama 演算子を説明します。<br>
~ (チルダ)は、右辺の結果を、マークされた要素のコンテントに設定します。<br>
= は、HTMLエスケープして出力します。<br>
従って、~= は右辺の結果をHTMLエスケープし、要素のコンテントに設定します。HTMLエスケープしたくなかったら、~== を使います。<br>
また、: (コロン)は、マークされた要素および子要素をその右辺の結果で置き換えます。従って、:== はHTMLエスケープせずに要素を置き換えます。

### コントローラからビューへのデータの引き渡し

コントローラで texport されたデータ（オブジェクト）をビューで使う場合は、tfetch メソッド（実はマクロ）で宣言する必要があります。引数には、変数の型と変数名を指定します。すると、指定された変数は texport される直前の状態と同じになるので、通常の変数と全く同じように使えます。上記のプレゼンテーションロジックの中で、実際そのように使われてます。<br>
使い方の例：

```
 コントローラ側：
   int foo;
   foo = ...
   texport(foo);

 ビュー側：
  tfetch(int, foo);
```

※ ２つ以上の変数に対しそれぞれ texport をコールすれば、ビューに引き渡すことができます

Otama システムは、これらテンプレートファイルとプレゼンテーションファイルを元に C++ コードを生成します。内部的には、tmake がそれを処理しています。その後、コードはコンパイルされ、ビューとして１つの共有ライブラリになります。なので、動作は非常に高速です。

#### HTML用語解説

要素（element）は、開始タグ (Start-tag)、コンテント (Content)、終了タグ (End-tag) の3つで構成されます。例として "\<p>Hello\</p>" という要素があったとすると、\<p> が開始タグ、Hello がコンテント、\</p> が終了タグになります。<br>
一般にコンテントのことを「内容」と呼ぶことの方が多いようですが、個人的に少々紛らわしいと思うので、ここではコンテントと書いています。

## モデルと ORM

TreeFrog では、モデルが ORM オブジェクトを含むという関係なので、has-a の関係です（ただし、２つ以上の ORM オブジェクトを持つようなモデルを作っても構いません）。他のほとんどのフレームワークでは、デフォルトで 「ORM オブジェクト＝モデル」 になっていますから、ここは違っていますね。とは言っても、できることは特に変わりません。TreeFrog では、モデルオブジェクトはORM オブジェクトの小さいラッパーであると理解してください。

TreeFrog には SqlObject という名の O/R マッパーがデフォルトで組み込まれています。<br>
C++ は静的型付け言語なので、型の宣言が必要です。生成された SqlObject  ファイル blogobject.h を見てみましょう。

半分ほどおまじないコードがありますが、テーブルのフィールドがパブリックなメンバ変数として宣言されています。構造体に近いですね。たったこれだけで、CRUD 相当のメソッド(create, findFirst, update, remove) が使えるようになります。それらのメソッドは TSqlObject  クラスと TSqlORMapper クラスに定義されています。

```c++
class T_MODEL_EXPORT BlogObject : public TSqlObject, public QSharedData
{
public:
    int id {0};
    QString title;
    QString body;
    QDateTime created_at;
    QDateTime updated_at;
    int lock_revision {0};

    enum PropertyIndex {
        Id = 0,
        Title,
        Body,
        CreatedAt,
        UpdatedAt,
        LockRevision,
    };

    int primaryKeyIndex() const override { return Id; }
    int autoValueIndex() const override { return Id; }
    QString tableName() const override { return QLatin1String("blog"); }

private:    /*** Don't modify below this line ***/      // ここから下はおまじないマクロ
    Q_OBJECT
    Q_PROPERTY(int id READ getid WRITE setid)
    T_DEFINE_PROPERTY(int, id)
    Q_PROPERTY(QString title READ gettitle WRITE settitle)
    T_DEFINE_PROPERTY(QString, title)
    Q_PROPERTY(QString body READ getbody WRITE setbody)
    T_DEFINE_PROPERTY(QString, body)
    Q_PROPERTY(QDateTime created_at READ getcreated_at WRITE setcreated_at)
    T_DEFINE_PROPERTY(QDateTime, created_at)
    Q_PROPERTY(QDateTime updated_at READ getupdated_at WRITE setupdated_at)
    T_DEFINE_PROPERTY(QDateTime, updated_at)
    Q_PROPERTY(int lock_revision READ getlock_revision WRITE setlock_revision)
    T_DEFINE_PROPERTY(int, lock_revision)
};
```

TreeFrog の O/R マッパーにはプライマリキーでの照会や更新を行うメソッドがありますが、SqlObject が持てるプライマリキーは primaryKeyIndex() メソッドで返す１つだけです。従って、複数プライマリキーをもつテーブルでは、必要に応じて修正し１つ返してください。<br>
TCriteria クラスを使うことで、より複雑な条件を指定してクエリを発行することも可能です。詳しくは各章で。

次に、モデルを見てみましょう。<br>
各プロパティのセッター/ゲッターと、オブジェクトの生成/取得の静的メソッドが定義されています。親クラスの TAbstractModel に保存 (save) と削除 (remove) のメソッドが定義されているので、結果として Blog クラスには CRUD 相当のメソッド（create, get, save, remove）が備わっています。

```c++
class T_MODEL_EXPORT Blog : public TAbstractModel
{
public:
    Blog();
    Blog(const Blog &other);
    Blog(const BlogObject &object);  // ORM オブジェクト指定のコンストラクタ
    ~Blog();

    int id() const;     // ここからセッター、ゲッターが並ぶ
    QString title() const;
    void setTitle(const QString &title);
    QString body() const;
    void setBody(const QString &body);
    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    int lockRevision() const;
    Blog &operator=(const Blog &other);

    bool create() { return TAbstractModel::create(); }
    bool update() { return TAbstractModel::update(); }
    bool save()   { return TAbstractModel::save(); }
    bool remove() { return TAbstractModel::remove(); }

    static Blog create(const QString &title, const QString &body); // オブジェクト生成
    static Blog create(const QVariantMap &values);                 // Hash でプロパティを渡してオブジェクト生成
    static Blog get(int id);                    // ID 指定でモデルオブジェクトを取得
    static Blog get(int id, int lockRevision);  // ID とlockRevision指定でモデルオブジェクトを取得
    static int count();             // ブログデータアイテムの量
    static QList<Blog> getAll();    // モデルオブジェクトを全取得
    static QJsonArray getAllJson(); // JSONスタイルにモデルオブジェクトを全取得

private:
    QSharedDataPointer<BlogObject> d;   // ORM オブジェクトのポインタを持つ

    TModelObject *modelData();
    const TModelObject *modelData() const;
};

Q_DECLARE_METATYPE(Blog)    // おまじない
Q_DECLARE_METATYPE(QList<Blog>)
```

ジェネレータで自動生成されたコードはそんなに多くないステップ数にも関わらず、基本的な機能ができあがっています。<br>
当然ながら生成されたコードは完全ではなく、実際のアプリケーションではさらに複雑な処理になるはずなので、そのままは使えないことが多いかもしれません。手直しが必要でしょう。ジェネレータは、コードを書く手間を少々省く程度のものと考えてください。

上記で説明したコードの裏では、クッキーの改ざんチェック、楽観的ロック、SQL インジェクション対策や認証トークンを使った CSRF 対策が機能しています。興味のある方、ソースをのぞいてみてください。

## サンプルブログアプリ作成デモ

<div class="img-center" markdown="1">

[![Video Demo – Sample blog Application Creation](http://img.youtube.com/vi/M_ZUPZzi9V8/0.jpg)](https://www.youtube.com/watch?v=M_ZUPZzi9V8)

</div>