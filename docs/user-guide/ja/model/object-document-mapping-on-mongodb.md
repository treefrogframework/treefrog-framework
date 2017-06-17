---
title: MongoDB の O/Dマッピング
page_id: "060.060"
---

## MongoDB の O/Dマッピング

MongoDB は、保存するデータを JSON ライクな形式で表現し、ドキュメントとして保存しています。このドキュメントとプログラミング言語のオブジェクトとを対応付ける機能のことを、オブジェクト-ドキュメントマッピング（O/Dマッピング）と呼びます。

上記の章で説明した O/Rマッピングと同様に、O/Dマッピングにおいても１件のドキュメントが１つのオブジェクトに対応付けされます。

MongoDB のドキュメントは JSON ライクな形式であることから、階層構造（ネスト）を持つことが可能なわけですが、O/Dマッピングは２階層以上のドキュメントに対応していません。１階層のドキュメントにのみ対応しています。例えば、次のような単純な形式にのみ対応しています。

```json
{
  "name": "John Smith",
  "age": 20,
  "email": "foo@example.com"
}
```

## 準備

前章を参考に、MongoDB の接続設定は済ませておいてください。

次のコマンドをアプリケーションルートディレクトリで実行し、O/D マッピングのベースとなるクラスを生成します。この例では foo というコレクションを生成しています。モデル名は Foo となります。

```
 $ tspawn mm  foo
  created   models/mongoobjects/fooobject.h
  created   models/foo.h
  created   models/foo.cpp
  updated   models/models.pro
```

次に、ドキュメントに保存されるデータを定義します。ファイル models/mongoobjects/fooobject.h を編集し、文字型の変数 title と body を追加することにします。

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
public:
    QString title;     // ← ここに追加
    QString body;      // ← ここに追加
    QString _id;
    QDateTime createdAt;
    QDateTime updatedAt;
    int lockRevision;
    enum PropertyIndex {
        Id = 0,
        CreatedAt,
        UpdatedAt,
        LockRevision,
    };
  :
```

変数 _id 以外の変数は必須ではありませんので、削除しても構いません。変数 _id は、MongoDB の ObjectID に相当するものなので、削除しないでください。

※ このオブジェクトは MongoDB へのアクセスを担当するもので、以降では "Mongoオブジェクト"と呼ぶことにします。

再び次のコマンドを実行し、追加した内容を他のファイルへ反映させます。

```
 $ tspawn mm  foo
```

変更があるファイルは全て 'Y' を入力します。<br>
これで、CRUD を備えたモデルの完成です。簡単ですね。

コントローラやビューも含めたスキャフォールドを生成する場合は、'tspawn mm foo' の代わりに次のコマンドを実行することができます。

```
 $ tspawn ms  foo
```

これでスキャフォールドが生成されました。コンパイルしてから、APサーバを実行してみます。

```
 $ treefrog -d -e dev
```

ブラウザで http://localhost:8800/foo/ にアクセスすれば、一覧画面が表示されます。この画面を起点にして、データの新規登録、編集、削除することができます。

このように、Mongo オブジェクトのクラスを編集することによって Mongo ドキュメントのレイアウトを自動的に定義／変更をするので、このファイルは（Railsでいうところの）マイグレーションファイルと似ていますね。

## Mongo オブジェクトを読み込む

スキャフォールディングで生成されたクラスを参考にしながら、Mongo オブジェクトを読み込む方法を説明します。

次は、オブジェクトIDをキーに Mongo オブジェクトを読み込む方法です。

```c++
QString id;
id = ...
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
```

## Mongo オブジェクトを生成する

通常のオブジェクトと同じように作り、プロパティを設定します。create()メソッドを呼び出すと、ドキュメントが MongoDB へ新規に生成されます。

```c++
FooObject foo;
foo.title = ...
foo.body = ...
foo.create();
```

オブジェクトIDは自動で生成されるので、何も設定しないでください。

## Mongo オブジェクトを更新する

読み込んだ Mongo オブジェクトに対して値を設定します。更新には、update()メソッドを使います。

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.title = ...
foo.update();
```

また、ドキュメントを保存する関数として、save() もあります。
これは、該当するドキュメントが MongoDB 内に**存在しなければ** create() メソッドを、**存在して**いれば update() メソッドを内部で呼び出します。

## Mongo オブジェクトを削除する

Mongo オブジェクトを削除すると、そのドキュメントが削除されます。削除には remove() メソッドを使います。

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.remove();
```

##### 補足

これまで説明したとおり、Mongo オブジェクトはORM オブジェクト（O/Rマッパーオブジェクト）と同じように使うことができます。コントローラの視点で見てみると、モデルクラスが提供する関数は同じなので、この２つの使い方に全く違いはありません。それらのオブジェクトはモデルの 'private' 領域に隠蔽されているのです。

つまり、モデルクラスの名前を重複しないように定義すれば、MongoDB と RDB へアクセスすることが可能なるわけで、データを複数の DB システムに容易に分散させることができるのです。正しく実装すれば、ボトルネックになりがちな RDB の負荷を一気に低下させることができます。<br>
ただし、分散させる場合には、データの性質によって RDB に保存すべきか、MongoDB に保存すべきかを検討するべきです。おそらく、トランザクションを使いたいかどうかがポイントになるでしょう。

このように、仕組みの違う２つデータベースシステムに簡単にアクセスすることができるので、Webアプリケーションとしてスケーラビリティの高いシステムを構築することができます。

Mongo オブジェクトクラスと ORM オブジェクトクラスの相違点：

Mongo オブジェクトクラスには、インスタンス変数として QStringList 型を定義することが可能です。つまり、次のように定義できるということです。

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
public:
    QString _id;
    QStringList  texts;
     :
```

※ ORM オブジェクトクラスにはQStringList 型を定義できません。