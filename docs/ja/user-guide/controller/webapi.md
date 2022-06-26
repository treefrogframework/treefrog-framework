---
title: Web API
page_id: "050.060"
---

## Web API

Web API とは、Web 上のシステム間インターフェース、つまりデータを受け渡しするための規約のことです。この Web API を実装することで、ブラウザなどから HTTP や HTTPS を通じてデータの受け渡しができるようになります。

Web API はサービス提供側が自由に実装するわけですが、多くのクラウドサービスでは、デザイン思想は REST スタイル、データフォーマットは JSON の Web API が提供されています。

## Web API をスキャフォールディング

TreeFrog では、１つテーブルからデータを CRUD する Web API の足場を簡単に作ることができます。  
例えば、次のようなテーブルがあったとします。

```
sqlite> .schema blog
CREATE TABLE blog (id INTEGER PRIMARY KEY AUTOINCREMENT, title VARCHAR(20), body VARCHAR(200), created_at TIMESTAMP, updated_at TIMESTAMP, lock_revision INTEGER);
```

このテーブルに対して、次のコマンドで Web API の足場を作ります。

```
$ tspawn api blog
DriverType:   QSQLITE
DatabaseName: db/dbfile
HostName:
Database opened successfully
  created   models/sqlobjects/blogobject.h
  created   models/objects/blog.h
  created   models/objects/blog.cpp
  updated   models/models.pro
  created   models/apiblogservice.h
  created   models/apiblogservice.cpp
  updated   models/models.pro
  created   controllers/apiblogcontroller.h
  created   controllers/apiblogcontroller.cpp
  updated   controllers/controllers.pro
```

コントローラやサービスクラスなどのソースファイルが生成されました。ここで作られる Web API のデータフォーマットは JSON になります。

Web API のエントリーポイントは次のようにコントローラ `apiblogcontroller.h` に定義されます。

```
class T_CONTROLLER_EXPORT ApiBlogController : public ApplicationController {
    Q_OBJECT
public slots:
    void index();                    // 一覧取得
    void get(const QString &id);     // 1件取得
    void create();                   // 新規登録
    void save(const QString &id);    // 保存（更新）
    void remove(const QString &id);  // 1件削除
};
```

これらのエントリーポイントは次のとおりです。

```
/apiblog/index
/apiblog/get/(id)
/apiblog/create
/apiblog/save/(id)
/apiblog/remove/(id)
```

## Web API の動作確認

ソースをビルドし、サーバを起動します。

```
 $ make
 $ treefrog -e dev -d
```

curl コマンドで Web API の動作確認してみましょう。

```
$ curl -sS http://localhost:8800/apiblog/index
{"data":[]}
```

何もデータが登録されていないので、空の JSON データが取得されました。

## エントリーポイント追加

デフォルトのエントリーポイントに加え、`config/routes.cfg` に項目を記述することでエントリーポイントを追加することができます。例えば、次のように記述します。

```
# Method   Entry-point               Function

get       /api/blog/index           ApiBlog.index
get       /api/blog/get/:param      ApiBlog.get
post      /api/blog/create          ApiBlog.create
post      /api/blog/save/:param     ApiBlog.save
post      /api/blog/remove/:param   ApiBlog.remove
```

正しく追加されたかをチェックします。

```
$ treefrog --show-routes
Available routes:
  get     /api/blog/index  ->  apiblogcontroller.index()
  get     /api/blog/get/:param  ->  apiblogcontroller.get(id)
  post    /api/blog/create  ->  apiblogcontroller.create()
  post    /api/blog/save/:param  ->  apiblogcontroller.save(id)
  post    /api/blog/remove/:param  ->  apiblogcontroller.remove(id)
```

GET や POST の他に PUT や DELETE も追加することができます。詳しくは [URL ルーティング](/ja/user-guide/controller/url-routing.html) のページを参照してください。

curl コマンドで追加したエントリーポイントの動作確認してみましょう。

```
$ curl -sS http://localhost:8800/api/blog/index    ← 追加したエントリーポイント
{"data":[]}
```

同じように、空の JSON データを取得することができました。

## 登録用の Web API

次にデータを 1 件登録してみましょう。curl コマンドで JSON データを POST します。

```
$ curl -sS -X POST -H "Content-Type: application/json" -d '{"title":"Hello","body":"hello world"}'  http://localhost:8800/api/blog/create
```

もう一度、一覧を取得してみます。

```
$ curl -sS http://localhost:8800/api/blog/index
{"data":[{"body":"hello world","createdAt":"2022-05-25T16:39:02.142","id":1,"lockRevision":1,"title":"Hello","updatedAt":"2022-05-25T16:39:02.142"}]}
```

今後は一覧のデータを正しく取得できました。

ID を指定してデータを取得してみましょう。

```
$ curl -sS http://localhost:8800/api/blog/get/1
{"data":{"body":"hello world","createdAt":"2022-05-25T16:39:02.142","id":1,"lockRevision":1,"title":"Hello","updatedAt":"2022-05-25T16:39:02.142"}}
```

正しく取得できました。

## 返却するプロパティを制限する

スキャフォールディング後のソースでは、DB レコードのカラムの全データが返却されていましたが、返却データ（プロパティ）を制限してみましょう。

サービスクラス `apiblogservice.cpp` を編集します。

```
QJsonObject ApiBlogService::index()
{
    auto blogList = Blog::getAll();
    QJsonObject json = { {"data", tfConvertToJsonArray(blogList, {"id", "title", "body"})} };  // ←ここ
    return json;
}

QJsonObject ApiBlogService::get(int id)
{
    auto blog = Blog::get(id);
    QJsonObject json = { {"data", blog.toJsonObject({"id", "title", "body"})} };  // ←ここ
    return json;
}
```

ビルド後に、curl コマンドで確認してみると、指定したプロパティのみ返却されています。

```
$ curl -sS http://localhost:8800/api/blog/index
{"data":[{"body":"hello world","id":1,"title":"Hello"}]}

$ curl -sS http://localhost:8800/api/blog/get/1
{"data":{"body":"hello world","id":1,"title":"Hello"}}
```

## まとめ

スキャフォールディングでシンプルな Web API を作成することができました。

実際のケースでは、このスキャフォールディングで作られた実装では不十分でしょう。例えば、2 つ以上のテーブルからデータを取得し、階層化された JSON データを返すことがあります。その場合はサービスクラスなどを適宜修正して、実装してください。
