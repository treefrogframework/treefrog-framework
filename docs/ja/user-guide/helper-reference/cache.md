---
title: キャッシュ
page_id: "080.035"
---

## キャッシュ

Webアプリケーションでは、ユーザからのリクエストのたびに、データベースへの問い合わせとテンプレートのレンダリングなどの処理が行われ、HTMLデータが生成されています。多くは大したオーバヘッドはかかりませんが、大量のリクエストをさばく大規模なサイトでは無視できないかもしれません。たとえ小規模のサイトであっても、計算コストのかかる処理をリクエストのたびに行う必要がないならば、処理される回数を削減できるかを検討しましょう。
このような場合に、処理された後のデータをキャッシュすることが有効な手段になります。


## キャッシュを有効にする

application.ini ファイルで Cache.SettingsFile のコメントアウト(#)を外し、キャッシュバックエンドのパラメータ 'Cache.Backend'に値を設定します。
```
Cache.SettingsFile=cache.ini

Cache.Backend=sqlite
```
この例では SQLite を設定しました。他に使えるバックエンドとして MongoDB や Redis を設定することができます。
それらを使う場合にはサーバを起動しておいてください


次に、cache.ini を編集して、データベースへの接続情報を設定します。
デフォルトでは次のようになっています。特に変更する必要はないと思います。
```
[sqlite]
DatabaseName=tmp/cachedb
HostName=
Port=
UserName=
Password=
ConnectOptions=
PostOpenStatements=PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA synchronous=NORMAL; VACUUM;
```

バックエンドに MongoDB や Redis を設定した場合は、該当する箇所に接続情報を設定してください。


## ページのキャッシュ

生成されたHTMLデータをキャッシュすることできます。
HTMLデータを生成するためにアクション名を指定しますが（省略も可能）、それをキャッシュするにはさらにキーと保存する時間を指定します。

render()の代わりに使う関数は renderAndStoreInCache(..) です。
キャッシュされたHTMLデータを送信するには、renderFromCache(..) を使います。

例えば、"index"ビューのHTMLデータを"index"というキーで10秒間キャッシュするには次のようにします。
```
    if (! renderFromCache("index")) {
        renderAndStoreInCache("index", 10, "index");
    }
```

indexアクションで実行されるならば 第３引数の"index"は省略できます。
詳しくは APIリファレンスを参照してください。

ユーザによってページの内容が異なる場合は、異なるキーでキャッシュするべきです。


## データのキャッシュ

キャッシュしたいのはページだけとは限りません。バイナリやテキストをキャッシュしたいケースもあります。
TCacheクラスを使い、データを保存、読出、削除することができます。

```
  Tf::cache()->set("key", "value", 10);
    :
    :
  auto data = Tf::cache()->get("key");
    :
```

