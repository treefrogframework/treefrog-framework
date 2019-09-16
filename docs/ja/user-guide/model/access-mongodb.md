---
title: MongoDBへのアクセス
page_id: "060.050"
---

## MongoDBへのアクセス

MongoDB は、オープンソースのドキュメント指向データベースであり、いわゆる NoSQL システムの１つです。

RDB ではデータを管理するためにあらかじめテーブル（スキーマ）を定義しておく必要がありますが、MongoDB にはその必要がありません。MongoDB では、データを「ドキュメント」と呼ばれる JSON ライクな形式（BSON）で表し、その集合を「コレクション」として管理します。ドキュメントにはシステムで一意のID (ObjectID)が割り当てられます。

MongoDB と RDB の階層構造について比較してみます。

<div class="table-div" markdown="1">

| MongoDB    | RDB      | 備考       |
|------------|----------|---------------|
| データベース | データベース | 用語としては同じ |
| コレクション | テーブル    |               |
| ドキュメント | レコード   |               |

</div><br>	

## 接続情報の設定

MongoDB はインストールが済んでおり、サーバが起動しているとします。<br>
MongoDB サーバと通信するための接続情報を設定しましょう。まず、config/application.ini にある次の行を編集します。

```ini
 MongoDbSettingsFile=mongodb.ini
```

次に、config/mongodb.ini を編集し、データベース名やホスト名を指定します。SQL データベースの設定ファイルと同じように、このファイルは dev, test, product の３つのセクションに分けられています。<br>
データベース名はなんでも構いません。存在していない場合、初回アクセスした時に生成されます。

```ini
 [dev]
 DatabaseName=foodb        # データベース名
 HostName=192.168.x.x      # IPアドレスまたはホスト名
 Port=
 UserName=
 Password=
 ConnectOptions=           # 今回は不使用
```

設定が正しいかチェックしましょう。MongoDB が動作している時に、次のコマンドをアプリケーションルートディレクトリで実行します。

```
 $ tspawn --show-collections
 DatabaseName: foodb
 HostName:     localhost
 MongoDB opened successfully
 -----------------
 Existing collections:
```

成功すれば、このように表示されるでしょう。

Web アプリケーションは、MongoDB と SQL データベースの**両方へ**アクセスすることが可能です。これにより、Web システムの負荷の増大に対して柔軟に対応することができるのです。

## ドキュメントを新規に生成する

MongoDB サーバへアクセスするには、TMongoQuery オブジェクトを使います。コンストラクタの引数にコレクション名を指定し、インスタンスを生成します。

MongoDB ドキュメントは QVariantMap オブジェクトで表されます。このオブジェクトに対しキーと値のペアをセットしていき、最後に insert() メソッドで MongoDBへ挿入します。

```c++
#include <TMongoQuery>
---
TMongoQuery mongo("blog");  // blogコレクションに対する操作
QVariantMap doc;

doc["title"] = "Hello";
doc["body"] = "Hello world.";
mongo.insert(doc);   // 新規作成
```

内部的には、insert() が実行されるタイミングで一意の ObjectID が割り振られます。

### 補足

この例が示すように、MongoDB との接続／切断の処理について、開発者は全く気にする必要がありません。コネクションの管理はフレームワークが行なっているからです。このような仕組みにすることで、コネクションを再利用することが可能となり、オーバヘッドのかかる接続／切断の回数が抑えられているのです。

## ドキュメントを読み込む

条件と一致するドキュメントを検索し、それらを１件ずつ読み込んでみます。検索条件もまた QVariantMap オブジェクトで表現されるので間違えないようにしてください。

次の例では、条件をセットしたオブジェクトを find() メソッドに引数として渡し、検索を実行します。検索条件にマッチするドキュメントが複数存在することを想定し、while 文でループ処理をしています。

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;

criteria["title"] = "foo";  // 検索条件
criteria["body"] = "bar";   // 検索条件
mongo.find(criteria);       // 検索実行
while (mongo.next()) {
    QVariantMap doc = mongo.value();   // ドキュメント取得
    // 何か処理する
}
```

※ これら２つの条件は AND 演算子で結ばれます

検索結果から１件だけ読み込むならば、findOne() メソッドを使用することができます。

```c++
QVariantMap doc = mongo.findOne(criteria);
```

次に、キー"num"の値が１0を超えるドキュメントを読み出すための条件を作ってみましょう。その比較演算子として、"$gt" を使います。

```c++
QVariantMap criteria;
QVariantMap gt;
gt.insert("$gt", 10);
criteria.insert("num", gt);   // 検索条件
mongo.find(criteria);    // 検索を実行
   :
```

比較演算子は次のとおりです。

* **$gt**: Greater than
* **$gte**: Greater than or equal to
* **$lt**: Less than
* **$lte**: Less than or equal to
* **$ne**:  Not equal
* **$in**: In
* **$nin**: Not in

### OR演算子

２つの以上の条件を OR 演算子 **$or** で結合してみます。

```c++
QVariantMap criteria;
QVariantList orlst;
orlst << c1 << c2 << c3;  // ３つの条件を追加
criteria.insert("$or", orlst);
   :
```

以上のように、TMongoQuery において検索条件は QVariantMap 型で表現されます。MongoDB では検索条件は JSON で表現されるので、クエリが実行される際、その QVariantMap オブジェクトがそのまま JSON オブジェクトに変換されることになります。従って、ルールどおりに記述すれば MongoDB で提供されている全ての演算子を指定することができます。効率的な検索が可能になるでしょう。

その他の演算子については [MongoDB ドキュメント](http://docs.mongodb.org/manual/reference/operator/nav-query/){:target="_blank"}をご覧ください。

## ドキュメントを更新する

MongoDB サーバから読み込んだドキュメントを更新してみます。次で示す update() メソッドは、条件にマッチする１件のドキュメントを更新するものです。

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["title"] = "foo";             // 検索条件を設定
QVariantMap doc = mongo.findOne(criteria);   // 1件取得
doc["body"] = "bar baz";               // ドキュメントの内容を変更

criteria["_id"] = doc["_id"];          // 検索条件にObjectIDを追加
mongo.update(criteria, doc);
```

ちょっと補足しますと、検索条件にマッチするドキュメントが複数存在してもそのドキュメントが確実に更新されるように、検索条件に ObjectID を追加しています。

また、条件に一致する全てのドキュメントを更新する場合は、updateMulti() メソッドを使用します。

```c++
mongo.updateMulti(criteria, doc);
```

## ドキュメントを削除する

条件に一致する全てドキュメントを削除します。

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["foo"] = "bar";
mongo.remove(criteria);    // 削除
```

１件のドキュメントを削除するにはオブジェクトIDを条件に指定します。

```c++
criteria["_id"] = "517b4909c6efa89aed288706";  // Removes by ObjectID
mongo.remove(criteria);
```
