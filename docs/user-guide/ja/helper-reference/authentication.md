---
title: 認証
page_id: "080.020"
---

## 認証

TreeFrog では、簡潔な認証の仕組みを提供しています。<br>
認証の機能を実装するためには、まず「ユーザ」を表現するモデルクラスを作る必要があります。ここでは、ユーザ名とパスワードだけのプロパティを持つ User クラスを作ってみます。次のようにテーブルを定義します。

```sql
 > CREATE TABLE user ( username VARCHAR(128) PRIMARY KEY, password VARCHAR(128) );
```

次に、アプリケーションルートディレクトリに移動して、ジェネレータコマンドでモデルクラスを生成します。

```
 $ tspawn usermodel user
   created  models/sqlobjects/userobject.h
   created  models/user.h
   created  models/user.cpp
   created  models/models.pro
```

usermodel オプション（u オプションでも可）を指定することで、TAbstractUser クラスを継承したユーザモデルクラスが作られます。

ユーザモデルクラスのユーザ名およびパスワードに対応するフィールド名は、それぞれ username、password がデフォルトになっていますが、変えることも可能です。例えば、それぞれ user_id 、pass という名でスキーマを定義した場合は、ジェネレータコマンドで次のように指定して、クラスを生成してください。

```
 $ tspawn usermodel user user_id pass
```

※ ただ単に末尾に加えるだけです。

通常のモデルクラスと違う点として、ユーザモデルクラスには次のような authenticate メソッドが追加されています。このメソッドはその名のとおり認証を行うメソッドで、処理としてはユーザ名をキーにして User オブジェクトを読み込み、パスワードを比較して、一致すればそのモデルオブジェクトを返しています。

```c++
User User::authenticate(const QString &username, const QString &password)
{
    if (username.isEmpty() || password.isEmpty())
        return User();

    TSqlORMapper<UserObject> mapper;
    UserObject obj = mapper.findFirst(TCriteria(UserObject::Username, username));
    if (obj.isNull() || obj.password != password) {
        obj.clear();
    }
    return User(obj);
}
```

これをベースにして修正するようにしてください。<br>
認証の処理は外部のシステムに委ねる場合もあるでしょうし、あるいはパスワードには md5 値を保存したいという要望もあるかもしれません。

## ログイン

ログイン／ログアウトの処理を行うコントローラを作りましょう。例として、form, login, logout の３つのアクションをもつ AccountController クラスを作ります。

```
 > tspawn controller account form login logout
   created  controllers/accountcontroller.h
   created  controllers/accountcontroller.cpp
   created  controllers/controllers.pro
```

スケルトンコードが生成されました。
form アクションでは、ログインフォームを表示させます。

```c++
void AccountController::form()
{
    userLogout();  // 強制的にログアウト
    render();   // formビューを表示
}
```

ここでは単にフォームを表示させましたが、すでにログインしていたら直ちに別の画面へリダイレクトさせることも可能です。要件に応じて対応してください。

ログインフォームのビュー views/account/form.erb を次のように作ります。ログインフォームのポスト先は login アクションとします。

```
<!DOCTYPE HTML>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
</head>
<body>
  <h1>Login Form</h1>
  <div style="color: red;"><%==$message %></div>
  <%== formTag(urla("login")); %>
    <div>
      User Name: <input type="text" name="username" value="" />
    </div>
    <div>
      Password: <input type="password" name="password" value="" />
    </div>
    <div>
      <input type="submit" value="Login" />
    </div>
  </form>
</body>
</html>
```

login アクションでは、ポストされたユーザ名とパスワードで行う認証処理を次のように書いてみます。認証が成功したら、userLogin メソッドを呼び出してユーザをシステムにログインさせています。

```c++
void AccountController::login()
{
    QString username = httpRequest().formItemValue("username");
    QString password = httpRequest().formItemValue("password");

    User user = User::authenticate(username, password);
    if (!user.isNull()) {
        userLogin(&user);
        redirect(QUrl(...));
    } else {
        QString message = "Login failed";
        texport(message);
        render("form");
    }
}
```

※ user.h ファイルをインクルードしてください。そうしないとコンパイルエラーになります。

ログインの処理が出来上がりました。 <br>
上の例では行なっていませんが、すでにログインしている状態で userLogin() メソッド呼ぶと、重複ログインのエラーになるので、その戻り値（bool）をチェックするのがベターでしょう。

また、userLogin() メソッドが成功すると、ユーザモデルのidentityKey()の戻り値がセッションの格納されます。デフォルト実装では、ユーザ名が格納されます。

```c++
QString identityKey() const { return username(); }
```

システムとして一意の情報なら別の値を使用しても構いません。他に、IDなどプライマリキーを格納してもよいかもしれません。<br>
ここで格納された値は identityKeyOfLoginUser() で取り出すことができます。

## ログアウト

ログアウトするには、アクションの中で userLogout メソッドを呼び出すだけです。

```c++
void AccountController::logout()
{
    userLogout();
    redirect(url("Account", "form"));  // ログインフォームへリダイレクト
}
```

## ログインのチェック

ログインしていないユーザからのアクセスを保護するには、コントローラのpreFilterをオーバライドし、その処理を書きます。

```c++
bool FooController::preFilter()
{
    if (!isUserLoggedIn()) {
        redirect( ... );
        return false;
    }
    return true;
}
```

preFilter メソッドが false を返すと、この後でアクションは処理されません。<br>
もし、多くのコントローラにおいてアクセスを保護したい場合は、ApplicationController クラスの preFilter に設定することができます。

## ログインユーザの取得

ログインしているユーザのインスタンスを取得しましょう。<br>
identityKeyOfLoginUser() メソッドでID情報を取得することができます。空文字の場合は、そのセッションでは誰もログインしていないことを指します。ログインしていれば、デフォルトの実装ではユーザ名を取得できます。

これをキーとするインスタンスを取得するメソッドをユーザモデルクラスに定義します。

```c++
User User::getByIdentityKey(const QString &username)
{
    TSqlORMapper<UserObject> mapper;
    TCriteria cri(UserObject::Username, username);
    return User(mapper.findFirst(cri));
}
```

コントローラでは次のように呼び出します。

```c++
QString username = identityKeyOfLoginUser();
User loginUser = User::getByIdentityKey(username);
```

**補足**

この章で説明したログインの実装は、セッションを使って実装されています。従って、セッションの有効期間がログインの有効期間となります。