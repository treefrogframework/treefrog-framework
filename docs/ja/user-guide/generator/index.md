---
title: ジェネレータ
page_id: "040.0"
---

## ジェネレータ

ここでは、ジェネレータコマンド tspawn の説明をします。

## スケルトンを生成

なによりもまず最初は、アプリケーションのスケルトンを作成しなければなりません。blogapp という名で作ってみましょう。<br>
コマンドラインから次のコマンドを入力します。Windows の場合は、TreeFrog Command Prompt 上で実行してください。

```
 $ tspawn new blogapp
```

実行すると、アプリケーションルートディレクトリをトップとしたディレクトリツリー、設定ファイル(ini)、プロジェクトファイル(pro) などが生成されました。ディレクトリはよく見かける名のものばかりです。ディレクトリとしては次のものが生成されます。

* controllers &nbsp;&nbsp;コントローラ
* models &nbsp;&nbsp;モデル
* views &nbsp;&nbsp;ビュー
* heplers &nbsp;&nbsp;ヘルパ
* config &nbsp;&nbsp;設定ファイル置き場
* db &nbsp;&nbsp;データベースファイル置き場（SQLite）
* lib &nbsp;&nbsp;ライブラリ
* log &nbsp;&nbsp;ログファイル置き場
* plugin &nbsp;&nbsp;プラグイン置き場
* public &nbsp;&nbsp;静的なファイル置き場（画像やJavascript）
* script &nbsp;&nbsp;スクリプト置き場
* test &nbsp;&nbsp;テスト用
* tmp &nbsp;&nbsp;一時ディレクトリ（アップロード直後のファイルなど）

## スキャフォールドを生成

スキャフォールドとは「足場」という意味で、ここでは CRUD 操作を行える基礎的な実装のことです。スキャフォールドに含まれるものは、コントローラ、モデル、ビューのソースファイルおよびプロジェクトファイル（pro）で、これらをベースに本格的な開発を始めるのが良いでしょう。

ジェネレータコマンドでスキャフォールドを作るためには、あらかじめデータベースにテーブルを定義し、設定ファイルにそのデータベース情報を記述する必要があります。では、テーブルを定義してみましょう。<br>
例：

```sql
> CREATE TABLE blog (id INTEGER PRIMARY KEY, title VARCHAR(20), body VARCHAR(200));
```

データベースとしてSQLite を使用する場合、データベースファイルはアプリケーションルートにある db ディレクトリへ作ってください。<br>
設定ファイル（database.ini）にデータベース情報を設定します。ジェネレータコマンドは dev セクションに設定された情報を参照します。

```ini
[dev]
driverType=QMYSQL
databaseName=blogdb
hostName=
port=
userName=root
password=root
connectOptions=
```

<div class="center aligned" markdown="1">

**設定一覧**

</div>

<div class="table-div" markdown="1">

| 項目           | 意味            | 備考                                                                                                                                                                                                              |
|----------------|--------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| driverType     | ドライバ名        | 選択肢は次のとおり：<br>- QDB2: IBM DB2<br>- QIBASE: Borland InterBase Driver<br>- QMYSQL: MySQL Driver<br>- QOCI: Oracle Call Interface Driver<br>- QODBC: ODBC Driver<br>- QPSQL: PostgreSQL Driver<br>- QSQLITE: SQLite version 3 or above |
| databaseName   | データベース名      | SQLite の場合はファイルパスを指定します。<br>例：  db/blogdb                                                                                                                                              |
| hostName       | ホスト名          | 空欄の場合は localhost                                                                                                                                                                                     |
| port           | ポート番号        | 空欄の場合は デフォルトポート                                                                                                                                                                                           |
| userName       | ユーザ名          |                                                                                                                                                                                                                      |
| password       | パスワード           |                                                                                                                                                                                                                      |
| connectOptions | 接続オプション | 詳細は Qt ドキュメント参照：<br>[QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"}                                                                                                                                             |

</div><br>

データベースドライバが Qt SDK に組み込まれていないとデータベースへアクセスできません。もし組み込まれていなければ、FAQ を参照して組み込んでください。あるいは、[ダウンロードページ](http://www.treefrogframework.org/ja/%E3%83%80%E3%82%A6%E3%83%B3%E3%83%AD%E3%83%BC%E3%83%89){:target="_blank"}からデータベースドライバをダウンロードし、組み込んでください。

そうしてからジェネレータコマンドを実行すると、足場が生成されます。コマンドは必ずアプリケーションルートディレクトリで実行してください。

```
 $ cd blogapp
 $ tspawn scaffold blog
 driverType: QMYSQL
 databaseName: blogdb
 hostName:
 Database open successfully
   created  controllers/blogcontroller.h
   created  controllers/blogcontroller.cpp
   ：
```

<br>
##### 結論：  データベースにスキーマを定義し、ジェネレータコマンドで足場を作れ。

### テーブル名とモデル名/コントローラ名の関係

ジェネレータが生成するクラスの名前はテーブル名に基づいて決められ、次のようなルールになります。

```
 テーブル名         モデル名      コントローラ名          Sqlオブジェクト名
 blog_entry   →    BlogEntry   BlogEntryController   BlogEntryObject
```

つまり、モデル名ではアンダースコアが消えて、その次の文字が大文字になります。<br>
単語の単数形／複数形の話は全く考える必要がありません。

## ジェネレータのサブコマンド

tspawn コマンドの usage です。

```
 $ tspawn -h
 usage: tspawn <subcommand> [args]
 Available subcommands:
   new (n)  <application-name>
   scaffold (s)  <model-name>
   controller (c)  <controller-name>
   model (m)  <table-name>
   sqlobject (o)  <table-name>
```

サブコマンドとして controller, model, sqlobject を指定すると、それぞれ、コントローラだけ、モデル（SqlObject 含む）だけ、SqlObject だけを生成することができます。

### ～～ コラム ～～

TreeFrog には、DB スキーマの変更とその差分管理をするための仕組みであるマイグレーション機能がありません。次の理由であまり必要ないと思っています。

1. マイグレーション機能を作ったとして、その学習コストがかかってしまう
2. DB の操作は SQL でやった方が全機能を享受できる
3. TreeFrog では、テーブルを変更したときに ORM オブジェクトクラスだけを再生成することができる
 → モデルクラスへも影響がでる場合があるので、それだけじゃ済まないが…
4. SQL コマンドの差分管理はフレームワーク側でやるメリットがあまりない（と思う）

どうでしょうかね？

## 命名規約

TreeFrog にはファイル名やクラス名の命名規約があります。ジェネレータを使うと、以下の規約でファイルやクラスが生成されます。

#### コントローラの命名規約

コントローラのクラス名は、「テーブル名＋Controller」となります。常に大文字で始まり、単語の区切りの'_'（アンダースコア）を消し、その区切りの先頭文字を大文字にします。例えば、次のようなクラス名になります。

* BlogController
* EntryCommentController

これらのファイルは、controllers ディレクトリに保存されます。その際のファイル名は、クラス名を全て小文字にして、拡張子(.h と .cpp)をつけたものになります。

#### モデルの命名規約

モデルのクラス名は、コントローラと同様に常に大文字で始まり、単語の区切りの'_'（アンダースコア）を消し、その区切りの先頭文字を大文字にします。例えば、次のようなクラス名になります。

* Blog
* EntryComment

これらのファイルは、models ディレクトリに保存されます。コントローラと同様に、ファイル名はモデル名を全て小文字にして、拡張子(.h と .cpp)をつけたものになります。<br>
Rails のように単語の単数形・複数形の変換はしません。

#### ビューの命名規約

ビューのテンプレートは、全て小文字で「アクション名＋拡張子」というファイル名で、「views/コントローラ名」 ディレクトリに生成されます。拡張子は、テンプレートシステムによって変わります。<br>
また、ビューをビルドすると、テンプレートを C++ コードに変換し views/_src ディレクトリにソースファイルを出力します。それらをコンパイルして、ビューの共有ライブラリが作られます。

#### CRUD

CRUD とは、Web アプリケーションにおける主要な４つの機能のことで、「Create （生成）」 「Read （読込）」 「Update （更新）」 「Delete （削除）」の頭文字をとっています。
ジェネレータコマンドで足場を作成すると、次の命名でコードを生成します。

<div class="center aligned" markdown="1">

**CRUD 対応表**

</div>

<div class="table-div" markdown="1">

|       | アクション    | モデル     | ORM       | SQL       |
| ----- |-----------|-----------|-----------|-----------|
| C	    | create	| create() [static]<br>create()  | create()	| INSERT    |
| R     | index<br>show | get() [static]<br>getAll() [static] | find() | SELECT |
| U	    | save	    | save()<br>update() | update()	| UPDATE    |
| D	    | remove	| remove()	| remove()	| DELETE    |

</div><br>

## T_CONTROLLER_EXPORT マクロについて

ジェネレータで作成したコントローラクラスには、T_CONTROLLER_EXPORTというマクロが追加されています。これは何なのでしょう。

Windowsでは、コントローラはまとめて１つの DLL になりますが、これらのクラスや関数を外部から利用可能にするために、__declspec(dllexport) というキーワードを付けて定義する必要があるのです。T_CONTROLLER_EXPORT マクロは、このキーワードに置換されます。

Mac OS X や Linux では、不要なキーワードなので T_CONTROLLER_EXPORT には何も定義されていません。

```
 #define T_CONTROLLER_EXPORT
```

このようにすることで、同じソースコードが複数のプラットフォームに対応できているのです。