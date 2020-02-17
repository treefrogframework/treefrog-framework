---
title: JSON/XMLのレスポンス処理
page_id: "050.050"
---

## JSON/XMLのレスポンス処理

外部のアプリケーションに対して情報を提供するために、JSON あるいは XML 形式でデータを出力したいケースがあります。これらの形式は Ajax の用途で使われるケースが多いでしょう。

## モデルの内容を JSON 形式で送る

外部のアプリケーションが JavaScript であるならば、JSON（JavaScript Object Notation）形式が扱いやすいです。本フレームワークで JSON を扱うには、Qt バージョン５以降が必要になります。

コントローラから１件の JSON オブジェクトを送る例を紹介します。

```c++
Blog blog = Blog::get(10);
renderJson(blog.toVariantMap());
```

たったこれだけです。ジェネレータで生成されたモデルには、それ自身を QVariantMap オブジェクトへ変換する toVariantMap() メソッドが用意されています。注意すべき点として、このメソッドはモデルがもつすべてのプロパティを対象として変換するので、もし隠蔽すべき情報がある場合は別にメソッドを実装するのが良いでしょう。

また、全ての Blog オブジェクトをリスト形式で送ってみましょう。

```c++
renderJson( Blog::getAllJson() );
```

これまた簡単！ ただし、データベースに Blog のレコードが大量にあると、とんでもなことになるので注意してください。

この他にも、次のメソッドが用意されています。 [APIリファレンス](http://treefrogframework.org/tf_doxygen/classTActionController.html){:target="_blank"}に少し情報があるのでご覧ください。

```c++
bool renderJson(const QJsonDocument &document);
bool renderJson(const QJsonObject &object);
bool renderJson(const QJsonArray &array);
bool renderJson(const QVariantMap &map);
bool renderJson(const QVariantList &list);
bool renderJson(const QStringList &list);
```

## モデルの内容を XML 形式で送る

モデルの内容をXML 形式で送る方法は JSON 形式の場合とほとんど変わりません。次のいずれかのメソッドを呼び出してください。

```c++
bool renderXml(const QDomDocument &document);
bool renderXml(const QVariantMap &map);
bool renderXml(const QVariantList &list);
bool renderXml(const QStringList &list);
```

もしこれらの出力する内容が要件と合わない場合は、新たにテンプレートを使って実装するようにしてください。実装方法については[ビューの章]({{ site.baseurl }}/ja/user-guide/view/index.html){:target="_blank"}で説明されるとおりですが、コントローラの中でレスポンスのコンテントタイプを設定することだけは忘れないでください。

```c++
setContentType("text/xml")
```