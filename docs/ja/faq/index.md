---
title: faq
page_id: "faq.00"
---

## FAQ

### Dockerイメージはないのでしょうか。

Docker Hub で公開していますのでお使いください。

[https://hub.docker.com/u/treefrogframework/](https://hub.docker.com/u/treefrogframework/){:target="_blank"}


### MySQL へのデータの挿入に失敗する

以下のようなエラーログが残っていたら、文字セットの設定が正しくない可能性があります。

```
 Incorrect string value: '\xE3\x81\x82' for column 'xxxx' at row 1 QMYSQL: Unable to execute query
```

文字セットの設定を確認してみてください。


### MySQL の文字セットを確認するコマンド

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

上記は UTF-8 の例です。
文字セットが全てそろっていないと（character_set_filesystem は binary でよい）、TreeFrog はインサートに失敗したり、文字化けしたり、正常に動作しませんでした。
もし設定が異なっていたら、MySQL のマニュアルを参照して設定し直してください。その設定が終わってから、データベースの作成、テーブルを作成するのが間違いないようです。
 

あるいは、MySQLなどの場合はテーブル作成時に文字コードを指定する。

```
 create table table_name ( … ) DEFAULT CHARSET=utf8;
```