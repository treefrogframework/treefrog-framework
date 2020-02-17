---
title: O/Rマッピング
page_id: "060.010"
---

## O/Rマッピング

モデルは、内部で O/R マッピングのオブジェクトを通じてデータベースにアクセスしています。このオブジェクト（ORM オブジェクトと呼ぶ）とレコードは基本的に1対1の関係になり、次のイメージで表されるでしょう。<br>
1件のレコードが１つのオブジェクトに関連付きます。1対多の関係もよくありますが、ここでは割愛します。

<div class="img-center" markdown="1">

![ORM]({{ site.baseurl }}/assets/images/documentation/orm.png "ORM")

</div>

繰り返しになりますが、モデルはブラウザに返す情報を集めたものであり、ORM オブジェクトは RDB へアクセスして操作するものと理解してください。

次の DBMS (ドライバ)をサポートしています。Qt がサポートしているものと同等です。

- MySQL
- PostgreSQL
- SQLite
- ODBC
- Oracle
- DB2
- InterBase

説明を進めるにあたり、オブジェクト指向言語と RDB における用語の対応を確認をしましょう。

<div class="center aligned" markdown="1">

**用語対応表**

</div>

<div class="table-div" markdown="1">

| オブジェクト指向	 | RDB    |
|--------------------|--------|
| クラス              | テーブル  |
| オブジェクト         | レコード |
| プロパティ           | フィールド （カラム）|

</div><br>


## データベース接続情報

データベースの接続情報を設定ファイル（config/database.ini）に指定します。設定ファイルの内容は最初 product, test, dev の３つセクションに分かれており、この文字列をWebアプリケーション起動コマンド(treefrog)のオプション（-e）で指定することで、データベースを切り替えることができます。また、必要に応じて新たにセクションを追加して構いません。

設定可能なパラメータは次のとおり。

<div class="table-div" markdown="1">

| パラメータ     | 説明                                                                    |
|----------------|--------------------------------------------------------------------------------|
| driverType     | ドライバタイプ<br>QMYSQL, QPSQL, QSQLITE, QODBC, QOCI, QDB2, QIBASE から選択   |
| databaseName   | データベース名<br>SQLite の場合はファイルパスを指定（例：db/dbfile）    |
| hostName       | ホスト名                                                                      |
| port           | ポート                                                                           |
| userName       | ユーザ名                                                                      |
| password       | パスワード                                                                       |
| connectOptions | 接続オプション<br>選択肢はQt ドキュメントの[QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"}を参照 |

</div><br>

こうしてWebアプリケーションを起動すると、システムが自動的にデータベース接続を管理します。開発者はデータベースのオープンやクローズの処理を行う必要はありません。

データベースにテーブルを作成したら、設定ファイルの dev セクションに接続情報を設定し、ジェネレータコマンドで「足場」を生成しておきましょう。

以下の節では、[チュートリアルの章]({{ site.baseurl }}/ja/user-guide/tutorial/index.html){:target="_blank"}で作った BlogObject クラスを例に説明します。

## ORM オブジェクトの読み込み

最も基本となる操作はプライマリキーを使ってテーブルを検索し、レコードの内容を読み込むことです。

```c++
int id;
id = ...
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
```

全てのレコードを読み込むこともできます。

```c++
TSqlORMapper<BlogObject> mapper;
QList<BlogObject> list = mapper.findAll();
```

大量のレコードが存在するときにその全てを読み込むと、メモリを食いつぶしてしまう恐れがあるので注意が必要です。 setLimit メソッドで上限を設定することができます。

ORM の内部では SQL文 が生成され発行されています。どのようなクエリが発行されたかを確認するには、[クエリログ]({{ site.baseurl }}/ja/user-guide/helper-reference/logging.html){:target="_blank"}を確認してください。

## イテレータ

検索結果に対して1件ずつ処理したい場合は、イテレータを使うことができます。

```c++
TSqlORMapper<BlogObject> mapper;
mapper.find();              // クエリを発行
TSqlORMapperIterator<BlogObject> i(mapper);
while (i.hasNext()) {       // イテレータで回す
    BlogObject obj = i.next();
    // do something ..
}
```

## 検索条件を指定して ORM オブジェクトの読み込み

検索条件は TCriteria クラスで指定します。Title フィールドが "Hello world"であるレコードを1件読み込むときは次のようにします。

```c++
TCriteria crt(BlogObject::Title, "Hello world");
BlogObject blog = mapper.findFirst(crt);
if ( !blog.isNull() ) {
    // レコードが存在する場合
} else {
    // レコードが存在しない場合
}
```

複数の条件を組み合わせることもできます。

```c++
// WHERE title = "Hello World" AND create_at > "2011-01-25T13:30:00"
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.add(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // AND演算子で末尾に追加

TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findFirst(crt);
   :
```

条件をOR演算子でつなぎたい時は、addOr メソッドを使います。

```c++
// WHERE title = "Hello World" OR create_at > "2011-01-25T13:30:00" )
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.addOr(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // OR演算子で末尾に追加
   :
```

addOr メソッドで条件を追加すると、それまでの条件句は括弧で囲まれます。add メソッドと addOr メソッドを組み合わせて使う場合は、呼ぶ順番に注意してください。

##### コラム

AND演算子とOR演算子はどちらが優先されて評価されるかを覚えていますか。

ご存知のとおり、AND演算子ですね。<br>
AND 演算子とOR演算子が混在した式は、AND演算子の式から評価され、その後にOR演算子の式が評価されるのです。意図しない順番で評価されないようにするには括弧で囲むことが必要です。

## ORM オブジェクトを生成する

通常のオブジェクトと同じように作り、プロパティを設定します。その状態をデータベースに保存するには create メソッドを使います。

```c++
BlogObject blog;
blog.id = ...
blog.title = ...
blog.body = ...
blog.create();  // データベースに保存
```

## ORM オブジェクトを更新する

ORM オブジェクトを更新するために、レコードを読み込んで ORM オブジェクトを作ります。プロパティを設定して、update メソッドで保存します。

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.title = ...
blog.update();
```

## ORM オブジェクトを削除する

ORM オブジェクトを削除することは、そのレコードを削除するということです。<br>
次のように、ORM オブジェクトに読み込んでから remove メソッドで削除します。

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.remove();
```

他の方法として、ORM オブジェクトを作らずに、条件に一致するレコードを一気に削除することもできます。

```c++
// Title フィールドが "Hello" であるレコードを削除する
TSqlORMapper<BlogObject> mapper;
mapper.removeAll( TCriteria(BlogObject::Title, tr("Hello")) );
```

## IDの自動連番

データベースシステムの中には、フィールドに対する自動連番機能をもつシステムがあります。例えば、MySQLでは AUTO_INCREMENT属性、PostgreSQLではserial型のフィールドが相当します。

自動で採番されるということは、モデルが値を新規登録や更新をする必要がありません。TreeFrog Framework はこの仕組みに対応しています。<br>
まず、自動連番のフィールドをもつテーブルを作成しておきます。それからジェネレータコマンドでモデルを生成すると、そのフィールドに対して更新をかけないモデルが作成されます。

MySQLの例：

```sql
 CREATE TABLE animal ( id INT PRIMARY KEY AUTO_INCREMENT,  ...
```

PostgreSQL の例：

```sql
 CREATE TABLE animal ( id SERIAL PRIMARY KEY, ...
```

## レコードの作成日時・更新日時を自動的に保存する

レコードの作成日時や更新日時といった情報を保存したいケースはよくあります。その実装は決まりきっていることから、ルールに則ったカラム名にあらかじめ定義しておけば、フレームワークが自動で処理してくれます。開発者はわざわざコードを書く必要がないということです。

テーブルに対し、作成日時、更新日時を保存するカラム名をそれぞれ、created_at、updated_at にするだけです。型は TIMESTAMP 型にします。このようなフィールドがあると、ORM オブジェクトは適切なタイミングでタイムスタンプを記録します。

<div class="table-div" markdown="1">

| 項目              | カラム名                   |
|------------------|---------------------------|
| 作成日時の保存     | created_at                |
| 更新日時の保存     | updated_at または modified_at |

</div><br>

自動的に日時を保存する仕組みは、データベース自体にもあります。さて、この仕事はデータベースでやるのがいいのでしょうか、フレームワークでやるのがいいのでしょうか。

どちらでも構わないと思いますが、もしフィールド名をこれらの名称に定義できればフレームワークに任せ、そうでなければデータベース側に任せることをお勧めします。

## 楽観的ロックを利用する

楽観的ロックとは、更新の時に行ロックをかけず、他から更新されていないことを検証しつつデータを保存することです。他から更新されていたら、自身の更新は諦めます。

テーブルに対してあらかじめロックリビジョンという名のカラムを用意し、整数を記録していきます。ロックリビジョンは更新のたびにインクリメントされるので、読込み時と更新時とでその値が違うということは他から更新されたことを意味します。その値が同じである場合のみ、更新処理を行います。こうすることで、安全に更新処理をすることができるのです。ロックをかけない分、僅かながら DB システムの省メモリと処理速度向上が期待できるというメリットがあります。

SqlObject で楽観的ロックを利用するには、テーブルに lock_revision という名のカラム（列）を integer 型で追加し、ジェネレータを使ってクラスを生成します。これだけで、TSqlObject::update メソッドと TSqlObject::remove メソッドを呼び出した時に楽観的ロックが作動します。

##### 結論： レコードを更新する必要があるなら、integer 型で lock_revision という名のカラムを追加せよ。
