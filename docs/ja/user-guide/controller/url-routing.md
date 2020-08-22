---
title: URLルーティング
page_id: "050.030"
---

## URLルーティング

URLルーティングとは、リクエストされた URL に対して呼び出すアクションを決定する仕組みのことです。ブラウザからリクエストを受け取ると、このルーティングルールに該当するURLかどうかチェックされ、該当すれば定義したアクションが呼び出されます。該当しなければ、デフォルトのルールによるアクションが呼び出されます。

繰り返しになりますが、デフォルトのルールは次のとおりでした。

```
 /コントローラ名/アクション名/引数１/引数２/...
```

では、ルーティングルールをカスタマイズしてみましょう。<br>
ルーティングの定義は config/routes.cfg に記述します。１つのエントリは、１行でディレクティブ、パス、アクションを並べて書きます。 ディレクティブは *match*, *get*, *post*, *put*, *delete* の中から1つ選択 します。<br>
なお、# ではじまる行はコメント行と見なされます。

例えば、次のように書きます。

```
 match  /index  Merge.index
```

この場合には、ブラウザから /index とリクエストされたら、POSTメソッドかGETメソッドか関係なしにMerge コントローラの index アクションに受け渡します。

次に、get ディレクティブを定義した場合です。

```
 get  /index  Merge.index
```

この場合では、GET メソッドで /index がリクエストされた時だけ、ルーティングを実施します。 POST メソッドでリクエストされた場合は拒否されます（アクションに受け渡しません）。

同様に、post ディレクティブを指定した場合は、POST メソッドのリクエストのみ有効です。GETメソッドのリクエストは拒否されます。

```
 post  /index  Merge.index
```

次は、アクションに引数を渡す方法についてです。ルーティングルールとして次のエントリを定義したとします。":params"というキーワードがポイントです。

```
 get  /search/:params  Searcher.search
```

この場合には、GETメソッドで /search/foo がリクエストされたら、Searcher コントローラの引数を１つ持つ search アクションが呼び出されます。その引数には "foo" が渡されます。<br>
同様に、/search/foo/bar がリクエストされたら、引数を２つ持つ search アクションが呼び出されます。第１引数、第２引数にはそれぞれ "foo"、"bar" が渡されます。

```
 /search/foo     ->  SearcherController の search("foo");
 /search/foo/bar ->  SearcherController の search("foo", "bar");
```

## ルーティングの表示

アプリをビルドした後に次のコマンドを実行することで、現在のルーティング情報を確認できます。
```
 $ treefrog --show-routes
 Available controllers:
   match   /blog/index  ->  blogcontroller.index()
   match   /blog/show/:param  ->  blogcontroller.show(id)
   match   /blog/create  ->  blogcontroller.create()
   match   /blog/save/:param  ->  blogcontroller.save(id)
   match   /blog/remove/:param  ->  blogcontroller.remove(id)
```
