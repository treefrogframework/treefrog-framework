---
title: ロギング
page_id: "080.050"
---

## ロギング

Webアプリは、以下の４つのログを出力します。

<div class="table-div" markdown="1">

| ログ          | ファイル名    | 内容                                                                                                                                                                                                                                                                                      |
|--------------|--------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| アプリログ      | app.log      | 	Webアプリのロギング。<br>開発者はここへログを出力します。出力方法は下記参照。                                                                                                                                                                                                    |
| アクセスログ   | access.log   | ブラウザからのアクセスをロギング。<br>静的ファイルへのアクセスも含む。                                                                                                                                                                                                                           |
| TreeFrogログ | treefrog.log | TreeFrogシステムのロギング。<br>システムが出力するログなので、エラーが発生した場合は何か情報が残っている場合あり。                                                                                                                                                   |
| クエリログ    | query.log    | データベースへ発行されたクエリログ。<br>設定ファイルのSqlQueryLogFileの値にファイル名を指定する。出力を停止する場合は空にする。<br>ログ出力にはオーバヘッドがあるので、Webアプリを正式に運用する際には出力を停止するのが良いでしょう。 |

</div><br>

## アプリログの出力

アプリログは、Webアプリのロギングに使用されます。アプリログを出力する場合は次のメソッドを使用します。

* tFatal()
* tError()
* tWarn()
* tInfo()
* tDebug()
* tTrace()

渡せる引数は printf 形式と同じで、フォーマット文字列と可変個の変数です。例えば、こんな感じに使います。

```c++
tError("Invalid Parameter : value : %d", value);
```

すると、次のようなログが log/app.log ファイルに出力されるでしょう。

```
2011-04-01 21:06:04 ERROR [12345678] Invalid Parameter : value : -1
```

フォーマット文字列の末尾には改行コードは不要です。

## ログのレイアウト変更

出力されるログのレイアウトを変更することが可能です。logger.ini 設定ファイルにある FileLogger.Layout パラメータに設定します。

```ini
# Specify the layout of FileLogger.
#  %d : date-time
#  %p : priority (lowercase)
#  %P : priority (uppercase)
#  %t : thread ID (dec)
#  %T : thread ID (hex)
#  %i : PID (dec)
#  %I : PID (hex)
#  %m : log message
#  %n : newline code
FileLogger.Layout="%d %5P [%t] %m%n"
```

ログレイアウトにある %d の部分にはログの発生日時が挿入されます。日時のフォーマットは FileLogger.DateTimeFormat パラメータに指定します。指定可能な形式は QDateTime::toString() に引数に渡す値と同じですので、詳しくは [Qt ドキュメント](http://doc.qt.io/qt-5/qdatetime.html){:target="_target"}をご覧ください。

```ini
# Specify the date-time format of FileLogger, see also QDateTime
# class reference.
FileLogger.DateTimeFormat="yyyy-MM-dd hh:mm:ss"
```

## ログ出力レベルの変更

ログの出力レベルを logger.ini にある次のパラメータで設定することができます。

```ini
# Outputs the logs of equal or higher priority than this.
FileLogger.Threshold=debug
```

この例では debug レベル以上のログが出力されます。

##### 結論： 開発で必要なデバッグログは tDebug() 関数で出力しておけ。
