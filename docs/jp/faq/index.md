---
title: FAQ
---

## FAQ

### SQLドライバを組み込む方法

Qt SDK に使いたい SQL ドライバが組み込まれてない場合があります。例えば、Qt SDK for windows (2010.05 版) には、MySQL の SQL ドライバが組み込まれていません。<br>
組み込まれていない SQL ドライバを使用すると、次のようなログが残っていることでしょう（ログは logディレクトリに保存されます）。

```
 QSqlDatabase:  driver not loaded
 QSqlDatabase: available drivers: QSQLITE QODBC3 QODBC
```

この場合、ソースをコンパイルして組み込む必要があります。次の qmake コマンドでは、環境に合わせてインクルードパス（INCLUDEPATH）とライブラリパス（LIB）を指定してください。

Windows の例：<br>
Windows の場合のみ、リリース版とデバッグ版を作る必要があります。他のプラットフォームではその必要はありません。リリース版のみで良いでしょう。

```
 > C:\Qt\2010.05\qt\src\plugins\sqldrivers\mysql
 > qmake "INCLUDEPATH+='C:/Program Files (x86)/MySQL/MySQL Server 5.5/include' "
　　"LIBS+='C:/Program Files (x86)/MySQL/MySQL Server 5.5/lib/libmysql.lib' "   
   "CONFIG+=release" mysql.pro
   　　　（実際には１行で記述）
 > mingw32-make install

 > qmake "INCLUDEPATH+='C:/Program Files (x86)/MySQL/MySQL Server 5.5/include' "
　　"LIBS+='C:/Program Files (x86)/MySQL/MySQL Server 5.5/lib/libmysql.lib' " 
   "CONFIG+=debug" mysql.pro
   　　　（実際には１行で記述）
 > mingw32-make install 
```

さらに、MySQL をインストールしたディレクトリの lib 下にある libmySQL.dll をパスのとおっているディレクトリへコピーします。例えば、C:\Windows にコピーします。<br>
以上で組み込み完了。

## MySQL へのデータの挿入に失敗する

以下のようなエラーログが残っていたら、文字セットの設定が正しくない可能性があります。

```
 Incorrect string value: '\xE3\x81\x82' for column 'xxxx' at row 1 QMYSQL: Unable to execute query
```

文字セットの設定を確認してみてください。

## MySQL の文字セットを確認するコマンド

mysql クライアントから次のコマンドを入力します。

```
 mysql> show variables like "char%";
 +--------------------------+----------------------------+
 | Variable_name            | Value                      |
 +--------------------------+----------------------------+
 | character_set_client     | utf8                       |
 | character_set_connection | utf8                       |
 | character_set_database   | utf8                       |
 | character_set_filesystem | binary                     |
 | character_set_results    | utf8                       |
 | character_set_server     | utf8                       |
 | character_set_system     | utf8                       |
 | character_sets_dir       | /usr/share/mysql/charsets/ |
 +--------------------------+----------------------------+
```

上記は UTF-8 の例です。<br>
文字セットが全てそろっていないと（character_set_filesystem は binary でよい）、TreeFrog はインサートに失敗したり、文字化けしたり、正常に動作しませんでした。<br>
もし設定が異なっていたら、MySQL のマニュアルを参照して設定し直してください。その設定が終わってから、データベースの作成、テーブルを作成するのが間違いないようです。
 
あるいは、MySQLなどの場合はテーブル作成時に文字コードを指定する。<br>

```
 create table table_name ( … ) DEFAULT CHARSET=utf8;
```