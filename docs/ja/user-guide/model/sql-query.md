---
title: SQLクエリ
page_id: "060.020"
---

## SQLクエリ

単純なレコードの読み込みであれば SqlObject の機能で十分ですが、複数のテーブルにかかるような複雑な処理では SQL のクエリ文を直接発行したいケースがあります。本フレームワークでは、プレースホルダを使うことで安全にクエリを生成することができます。

次は ODBC 形式のプレースホルダを使ったクエリを発行する例です。

```c++
TSqlQuery query;
query.prepare("INSERT INTO blog (id, title, body) VALUES (?, ?, ?)");
query.addBind(100).addBind(tr("Hello")).addBind(tr("Hello world"));
query.exec();  // クエリ実行
```

次は名前付きプレースホルダを使った例です。

```c++
TSqlQuery query;
query.prepare("INSERT INTO blog (id, title, body) VALUES (:id, :title, :body)");
query.bind(":id", 100).bind(":title", tr("Hello")).bind(":body", tr("Hello world"));
query.exec();  // クエリ実行
```

クエリ結果からデータを取り出す方法は、Qt の QSqlQuery クラスと同じです。

```c++
TSqlQuery query;
query.exec("SELECT id, title FROM blog");  // クエリ実行
while (query.next()) {
int id = query.value(0).toInt(); // 最初のフィールドをint型へ変換
    QString str = query.value(1).toString(); // 2番目のフィールドをQString型へ変換
    // do something
}
```

TSqlQueryクラスは Qt の[QSqlQuery クラス](https://doc.qt.io/qt-5/qsqlquery.html){:target="_blank"}を継承しているので、同じメソッドが使用できます。詳しくは、Qtドキュメントを参照ください。

##### 結論： プレースホルダを使ってクエリを生成せよ。

実際に、どんなクエリが実行されたのかを[クエリログ]({{ site.baseurl }}/ja/user-guide/helper-reference/logging.html){:target="_blank"}で確認することができます。

## クエリをファイルから読み込む

ソースコードにクエリ文を書くと修正するたびにコンパイルが必要になるので、アプリの開発期間中は少々手間になるかもしれません。これを軽減するために、クエリ文だけを別ファイルに書いておき、実行時にロードする仕組みがあります。

sql ディレクトリにそのファイルを配置します（但しディレクトリは application.ini で変更可）。ここでは仮に insert_blog.sql として、次の内容を記述しておきます。

```sql
INSERT INTO blog (id, title, body) VALUES (?, ?, ?)
```

次はソースコードです。load メソッドでこの insert_blog.sql ファイルを読み込みます。

```c++
TSqlQuery query;
query.load("insert_blog.sql")
query.addBind(100).addBind(tr("Hello")).addBind(tr("Hello world"));
query.exec();  // クエリ実行
```

load メソッドの内部ではキャッシュが働きます（ただし [MPM]({{ site.baseurl }}/ja/user-guide/performance/index.html){:target="_blank"} に thread モジュールを適用した場合のみ）。最初の1回だけファイルからクエリ文を読み込むと、これ以降メモリ上のキャッシュを使うようになるので高速に動作します。

ファイルを修正したら、クエリ文を読み込ませるためにサーバを再起動しましょう。

```
 $ treefrog -k abort ;   treefrog -d  -e dev
```

あるいはこうです。

```
 $ treefrog -k restart
```

## クエリの結果からORMオブジェクトを取得する

上記の方法では、クエリの結果から１フィールド毎に値を取り出す必要がありますが、次の方法ではレコードを ORM オブジェクトとして取り出すことができるのです。

TSqlQueryMapper オブジェクトを使ってクエリを実行します。その結果からイテレータを使って ORM オブジェクトを取り出します。SELECT 文では 'blog.*' と指定し、すべてのフィールドを選択対象とするのがポイントです。

```c++
TSqlQueryORMapper<BlogObject> mapper;
mapper.prepare("SELECT blog.* FROM blog WHERE ...");
mapper.exec();  // クエリ実行

TSqlQueryORMapperIterator<BlogObject> it(mapper);
while (it.hasNext()) {
    BlogObject obj = it.next();
    // do something
     :
}
```

もし１件しか取り出す必要がなければ、execFirst() メソッドで結果を取得することができます。