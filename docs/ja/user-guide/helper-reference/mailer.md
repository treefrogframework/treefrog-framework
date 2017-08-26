---
title: メーラー
page_id: "080.040"
---

## メーラー

TreeFrog Framework には、メーラー（メールクライアント）が搭載されており、SMTP によるメール送信が可能です。今のところ(v1.0では)、SMTP によるメール送信のみ可能です。メールメッセージの作成するのに ERB テンプレートを使用します。

まずは、次のコマンドでメールのスケルトンを作ってみましょう。

```
 $ tspawn mailer information send
   created  controllers/informationmailer.h
   created  controllers/informationmailer.cpp
   created  controllers/controllers.pro
   created  views/mailer/mail.erb
```

controllers ディレクトリに InformationMailer クラスと、views ディレクトリに mail.erb という名前のテンプレートが作成されました。

作成された mail.erb を開いて、次の内容で保存します。

```
 Subject: Test Mail
 To: <%==$ to %>
 From: foo@example.com

 Hi,
 This is a test mail.
```

空行を挟んで、上がメールヘッダ、下が本文になります。ヘッダには件名、宛先などを指定します。任意のヘッダフィールドを追加可能です。ただし、Content-Type や Date フィールドについては自動的に追加されるので、わざわざ書く必要はありません。

もし日本語などのマルチバイト文字を使う場合は、設定ファイルの InternalEncoding に設定したエンコーディング（デフォルト：UTF-8）でファイルを保存してください。

InformationMailer クラスの send メソッドの最後で deliver() メソッドを呼び出します。

```c++
void InformationMailer::send()
{
    QString to = "sample@example.com";
    texport(to);
    deliver("mail");   // ← mail.erb テンプレートでメール送信
}
```

これで、外部のクラスからを呼ぶことができるようになりました。アクションの中に次のコードを記述することで、メールの送信処理を行います。

```c++
InformationMailer().send();
```

※ 実際にメールを送信するには、下記の「SMTP設定」を行なってください。

テンプレートを使わず、直接メールを送信する場合は TSmtpMailer::send() メソッドを使用することができます。

## SMTP 設定

上記のコードには SMTP に関する設定情報がありません。SMTP に関する情報は application.ini ファイルに設定してください。<br>
次の項目があります。

```ini
# Specify the connection's host name or IP address. （SMTPサーバ）
ActionMailer.smtp.HostName=smtp.example.com

# Specify the connection's port number.  （ポート番号）
ActionMailer.smtp.Port=25

# Enables SMTP authentication if true; disables SMTP
# authentication if false.
ActionMailer.smtp.Authentication=false

# Specify the user name for SMTP authentication.
ActionMailer.smtp.UserName=

# Specify the password for SMTP authentication.
ActionMailer.smtp.Password=

# Enables the delayed delivery of email if true. If enabled, deliver() method
# only adds the email to the queue and therefore the method doesn't block.
ActionMailer.smtp.DelayedDelivery=false
```

SMTP 認証を行う場合、ActionMailer.smtp.Authentication=**true** を指定します。<br>
認証方式には CRAM-MD5, LOGIN, PLAIN が実装されており、この優先度で自動的に認証処理が行われます。

本フレームワークでは、SMTPSによるメール送信はサポートしていません。

## メールの遅延送信

SMTP によるメール送信は外部のサーバとデータの受け渡しをすることから、他に比べて時間のかかる処理です。メールを送る処理は後にまわして、先にHTTPレスポンスを返すことができます。

次のとおりに *application.ini* ファイルを編集します。

```ini
ActionMailer.smtp.DelayedDelivery=true
```

こうすることで、deliver() メソッドは単にデータをキューイングするだけのノンブロッキング関数となります。HTTPレスポンスを返した後にメール送信の処理が行われます。

**補足：**<br>
遅延送信を設定しないと（falseの場合）、deliver() メソッドは SMTP の処理が完了またはエラーになるまでブロックします。