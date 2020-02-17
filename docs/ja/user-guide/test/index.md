---
title: テスト
page_id: "120.0"
---

## テスト

アプリの開発の過程において、テストという作業は欠かせないものです。同じようなことを繰り返し確認するテストは退屈なものなので、自動化できるところはそうしてしまいましょう。

## モデルのユニットテスト

本節では、モデルが正しく動作するかチェックしてみます。テストフレームワークは Qt 付属の TestLib を踏襲しているので、[そちらのドキュメント](http://doc.qt.io/qt-5/qtest-overview.html){:target="_target"}も一度ご覧ください。

[チュートリアル]({{ site.baseurl }}/ja/user-guide/tutorial/index.html){:target="_blank"}で作成した Blog モデルのテストコードを作ってみましょう。あらかじめ、モデルの共有ライブラリは作成しておいてください。<br>
まず、test ディレクトリに作業ディレクトリを作成します。

```
 $ cd test
 $ mkdir blog
 $ cd blog
```

Blog モデルの生成と読込みのテストケースを作ってみます。<br>
テストを実施するクラス名は TestBlog とします。次のような内容のソースコードを testblog.cpp というファイル名で保存します。

```c++
#include <TfTest/TfTest>
#include "models/blog.h"    // モデルクラスのインクルード

class TestBlog : public QObject
{
    Q_OBJECT
private slots:
    void create_data();
    void create();
};

void TestBlog::create_data()
{
    // テストデータの定義
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("body");

    // テストデータに追加
    QTest::newRow("No1") << "Hello" << "Hello world.";
}

void TestBlog::create()
{
    // テストデータの取込
    QFETCH(QString, title);
    QFETCH(QString, body);

    // テストのロジック
    Blog created = Blog::create(title, body);
    int id = created.id();
    Blog blog = Blog::get(id);  // IDでモデルを取得

    // 実行結果の検証
    QCOMPARE(blog.title(), title);
    QCOMPARE(blog.body(), body);
}

TF_TEST_MAIN(TestBlog)   // 作成したクラス名を指定
#include "testblog.moc"      // おまじない。拡張子を .moc にする
```

補足すると、この中でテストを実行するのは create() メソッドであり、実際に戻り値を検証するのは QCOMPARE マクロです。create_data() メソッドは、テストデータを create() メソッドへ渡す役目をします。メソッド名の末尾には '_data' をつけるのがルールです。

この例で、create_data() メソッドでは次の処理を行なっています。

* QTest::addColumn() 関数で、テストデータの型と名前を定義する
* QTest::newRow() 関数で、テストデータに追加する

create() メソッドでは次の処理を行なっています。

* テストデータを取り込む
* テストロジックを実行する
* その実行結果が正しいか検証する

次に、Makefile を作成するためのプロジェクトファイルを作ります。ファイル名は testblog.pro とし、次の内容を保存します。

```
 TARGET = testblog
 TEMPLATE = app
 CONFIG += console debug c++14
 CONFIG -= app_bundle
 QT += network sql testlib
 QT -= gui
 DEFINES += TF_DLL
 INCLUDEPATH += ../..
 LIBS += -L../../lib -lmodel
 include(../../appbase.pri)
 SOURCES = testblog.cpp      # ファイル名を指定する
```

プロジェクトファイルを保存したら、そのディレクトリで次のコマンドを実行してバイナリを作成します。

```
 $ qmake
 $ make
```

次に、テストを実行するための設定を行います。<br>
テストコマンドは各種設定ファイルを参照する必要があるため、その直下に位置するよう *config* ディレクトリへのシンボリックリンクを作成します。データベースとして SQLite を使用している場合は *db* ディレクトリへのシンボリックリンクも作成します。

```
 $ ln -s  ../../config  config
 $ ln -s  ../../db  db
```

Windows の場合、テストの EXE ファイルは debug ディレクトリに作られるので、そこでシンボリックリンクを作ります。「ショートカット」ではないので注意してください。シンボリックリンクを作るには、**管理者権限**で起動したコマンドプロンプトからコマンドを実行する必要があります。

```
 > cd debug
 > mklink /D  config  ..\..\..\config
 > mklink /D  db  ..\..\..\db
```

さらに、Blogモデルを包含する共有ライブラリへパスを通します。<br>
Linux の場合には、次のように環境変数を設定します。

```
 $ export  LD_LIBRARY_PATH=/path/to/blogapp/lib
```

Windows の場合には、PATH 変数に設定を追加します。

```
 > set PATH=C:\path\to\blogapp\lib;%PATH%
```

次に、データベースの接続情報を確認をします。ユニットテストでは、データベース設定ファイル（*database.ini*）にある test セクションの接続情報が使用されます。

```
 [test]
 DriverType=QMYSQL
 DatabaseName=blogdb
 HostName=
 Port=
 UserName=root
 Password=
 ConnectOptions=
```

以上で設定は完了です。それでは、テストを実行してみましょう。テストが成功すれば、次のようなメッセージが表示されます。Windows の場合には、TreeFrog Command Prompt 上で実行してください。

```
$ ./testblog
Config: Using QtTest library 5.5.1, Qt 5.5.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 5.4.0 20160609)
PASS   : TestBlog::initTestCase()
PASS   : TestBlog::create(No1)
PASS   : TestBlog::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestBlog *********
```

もし期待した結果と一致しなければ、次のようなメッセージが表示されるでしょう。

```
********* Start testing of TestBlog *********
Config: Using QtTest library 5.5.1, Qt 5.5.1 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 5.4.0 20160609)
PASS   : TestBlog::initTestCase()
FAIL!  : TestBlog::create(No1) Compared values are not the same
   Actual   (blog.body()): "foo."
   Expected (body): "Hello world."
   Loc: [testblog.cpp(35)]
PASS   : TestBlog::cleanupTestCase()
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestBlog *******
```

モデルごとにテストケースを作成し、どんどんテストを行なってください。モデルがきちんと動作するかどうかが、Web アプリの要（かなめ）となるのです。
