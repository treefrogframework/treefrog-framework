---
title: アクセス制御
page_id: "050.040"
---

## アクセス制御

Webサイトにおけるアクセス制御は２通りの方法が考えられます。接続元のホスト（IPアドレス）による制御とユーザ認証による制御です。ホスト制御は TreeFrog には実装されていないので、Webサーバ（Apache/nginx）をプロキシサーバに置くなどして必要に応じて設定してください。

以下ではユーザによる制御について述べます。

## ユーザアクセス制御

Webサイトの中には、誰でもアクセスできるページと決まったユーザだけがアクセスできるページがあります。管理用ページはその権限を持つ者しかアクセスできないものです。このようなケースで、次のような方法でアクセスを拒否することができます。

まず、[認証]({{ site.baseurl }}/ja/user-guide/helper-reference/authentication.html){:target="_blank"}の章を参考にして、ユーザモデルクラスを作ります。<br>
アクセスを制限したいページについては、ログイン認証を必須とします。そうすることでユーザモデルのインスタンスが得られることになります。

次に、コントローラの setAccessRules()メソッドをオーバライドして、ユーザIDあるいはグループによるアクションへのアクセス許可／拒否のアクセスルールを設定します。ユーザID、グループはそれぞれユーザモデルクラスの identityKey()メソッド、groupKey()メソッドの戻り値を指しています。

```c++
void FooController::setAccessRules()
{
   setDenyDefault(true);
   QStringList allowed;
   allowed << "index" << "show" << "entry" << "create";
   setAllowUser("user1", allowed);
     :
}
```

アクセスを許可するには allowUser() や allowGroup() を使い、アクセスを拒否するには denyUser() や denyGroup() を使って定義します。第１引数には、ユーザIDまたはグループを指定します。第２引数には、アクション名またはそのリスト(QStringList)を指定します。

アクセスルールを定義していないユーザからの許可／拒否については、デフォルトの設定が使われます。これは setAllowDefault()メソッドか、 setDenyDefault()メソッドで設定します。

```c++
setDenyDefault(true);
```

次に、アクセスしてきたユーザを検証するロジックです。コントローラのpreFilter()メソッドをオーバライドし、アクセスを拒否するユーザの場合は false を返す。

```
bool FooController::preFilter()
{
   ApplicationController::preFilter();
    :
    :  // ユーザモデルのインスタンスを取得
    :
   if (!validateAccess(&loginUser)) {  // 定義したアクセスルールで検証する
       renderErrorResponse(403);
       return false;
   }
   return true;
}
```

preFilter()メソッドは false を返すと、この後アクションは実行されません。これでアクセスを拒否することになります。<br>
この例では renderErrorResponse()メソッドを使うことで、静的なエラーページ（*public/403.html*）を表示させています。