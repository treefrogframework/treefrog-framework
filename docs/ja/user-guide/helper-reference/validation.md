---
title: バリデーション
page_id: "080.030"
---

## バリデーション

リクエストとして送られてくるデータは、開発者が望んだ形式で送られてこないかもしれません。数値を入力すべきフィールドにアルファベットを入力するユーザがいるかもしれません。たとえクライアントサイドのJavascriptで検証するように実装したとしても、リクエストの内容を改ざんするのは簡単なことなので、サーバサイドでその内容を検証する仕組みは必須なのです。

前述のとおり、受信したリクエストデータはハッシュ形式で表現されます（取得方法については[こちらの章]({{ site.baseurl }}/ja/user-guide/controller/index.html){:target="_blank"}をご覧ください）。通常、リクエストデータをモデルにセットする前に、それぞれの値が正しい形式であるかを検証します。

まず、blog 向けのリクエストデータ（ハッシュ）を検証するバリデーションクラスのスケルトンを生成します。アプリケーションルートに移動して、次のコマンドを実行します。

```
 $ tspawn validator blog
   created   helpers/blogvalidator.h
   created   helpers/blogvalidator.cpp
   updated   helpers/helpers.pro
```

生成された BlogValidator クラスのコンストラクタで、バリデーションルールを設定します。例えば、*title* 変数の文字列の長さが4文字以上20文字以下というルールは、次のように書きます。

```c++
BlogValidator::BlogValidator() : TFormValidator()
{
   setRule("title", Tf::MinLength, 4);
   setRule("title", Tf::MaxLength, 20);
}
```

この第２引数である enum 値として、入力必須、文字列最大長/最小長、整数最大値/最小値、日付形式か、メールアドレス形式か、ユーザ定義ルール（正規表現）などから１つ指定できます （これらは tfnamespace.h に定義されている）。<br>
setRule() の第４引数には、検証エラー時のメッセージを設定することができます。もしメッセージを指定しなければ、*config/validation.ini* ファイルに定義されたメッセージが設定されます。

「入力必須」のルールについては暗黙的に設定されます。もし「入力必須」**でない**ようにするには、次のように記述します。

```c++
setRule("title", Tf::Required, false);
```

<div class="center aligned" markdown="1">

**設定可能なルール**

</div>

<div class="table-div" markdown="1">

| 値           | 説明                 |
|--------------|-------------------------|
| Required     | 入力必須                 |
| MaxLength    | 文字列の最大長            |
| MinLength    | 文字列の最小長            |
| IntMax       | 最大値（int）            |
| IntMin       | 最小値（int）            |
| DoubleMax    | 最大値（double）         |
| DoubleMin    | 最小値（double）         |
| EmailAddress | Eメールアドレス           |
| Url          | URL形式                 |
| Date         | 日付形式                 |
| Time         | 時間形式                 |
| DateTime     | 日時形式                 |
| Pattern      | 正規表現                 |

</div><br>

ルール設定をしたら、コントローラの中で使ってみましょう。該当するヘッダファイルをインクルードしておいてください。<br>
フォームから取得したリクエストデータを検証します。検証エラーになった場合に、エラーメッセージを取得します。

```c++
QVariantMap blog = httpRequest().formItems("blog");
BlogValidator validator;
if (!validator.validate(blog)) {
    // 検証エラーになったルールのメッセージ取得
    QStringList errs = validator.errorMessages();
       :
}
```

通常、複数のルールを設定するはずなので、エラーメッセージも複数になります。１つ１つ処理するのは少々面倒なものです。<br>
次のメソッドを使えば、検証エラーのメッセージを一括でエクスポートする（ビューに渡す）ことができます。第2引数には、エクスポートオブジェクトの変数名へのプレフィックスを指定します。

```c++
exportValidationErrors(valid, "err_");
```

##### 結論： フォームのデータはルールを設定し、validate() で検証せよ。

## カスタムバリデーション

上で説明した内容は静的なバリデーションの方法です。ある値に応じて、ある別の値の許容範囲が変わるような動的なケースでは使えません。この場合、validate() メソッドをオーバライドして、自由にバリデーションのコードを書くことができます。

次はサンプルコードです。

```c++
bool FooValidator::validate(const QVariantMap &hash)
{
    bool ret = THashValidator::validate(hash);  // ← 静的ルールのバリデーション
    if (ret) {
        QDate startDate = hash.value("startDate").toDate();
        QDate endDate = hash.value("endDate").toDate();
        if (endDate < startDate) {
            setValidationError("error");
            return false;
        }
          :
          :
    }
    return ret;
}
```

startDate の値と endData の値を比較して、正しくない場合に検証エラーにしています。