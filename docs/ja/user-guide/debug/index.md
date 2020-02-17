---
title: デバッグ
page_id: "110.0"
---

## デバッグ

作成したソースコードをビルドすると、４つの共有ライブラリが生成されます。これらが Web アプリの実体です。TreeFrog のアプリケーションサーバ（APサーバ）は起動時にこれらを読み込み、ブラウザからのアクセスを待ちます。

この Web アプリをデバッグするのは、通常の共有ライブラリをデバッグするのと同じです。まずは、デバッグモードでソースコードをコンパイルしておきましょう。アプリケーションルートディレクトリで、次のコマンドを実行します。

```
 $ qmake -r "CONFIG+=debug"
 $ make clean
 $ make
```

デバッグでは、プラットフォームに応じて次の設定を使用します。

#### Linux / Mac OS Xの場合：

| オプション                                    | 値                                          |
|-----------------------------------------------|------------------------------------------------|
| 実行コマンド                                   | tadpole                                        |
| コマンド引数                                  | \--debug -e dev (アプリケーションルートの絶対パス) |
| LD_LIBRARY_PATHの設定<br>（Mac OS X では不要） | Webアプリのlibディレクトリを指定  |


#### Windowsの場合：

| オプション     | 値                                                            |
|---------------|---------------------------------------------------------------|
|  実行コマンド  | tadpole**d**.exe                                             |
| コマンド引数   | \--debug -e dev (アプリケーションルートの絶対パス)             |
| PATH 変数      | TreeFrog の bin ディレクトリ (c:\TreeFrog\x.x.x\bin) を先頭に追加<br>MySQL やPostgreSQLなどを使う場合はクライアントDLLのあるディレクトリも追加 |
| TFDIR 変数    | TreeFrog ディレクトリ(c:\TreeFrog\x.x.x)を設定 |

※ "x.x.x" はTreeFrog のバージョン

次で、これらの項目を設定していきましょう。

## Qt Creator によるデバッグ

ここでは、Qt Creator を使ったデバッグを紹介します。他のデバッガでもやり方は基本的に変わらないと思います。
まず、アプリケーション設定ファイルにある [MPM]({{ site.baseurl }}/ja/user-guide/performance/index.html){:target="_blank"} には thread を設定しておいてください。

```
 MultiProcessingModule=thread
```

アプリのソースコードを Qt Creator にインポートします。 [ File ] – [ Open File or Project... ] をクリックし、ファイル選択画面でアプリケーションルートにあるプロジェクトファイルを選択します。[ Configure Porject ]ボタンをクリックし、プロジェクトをインポートします。次は blogapp という名のプロジェクトをインポートする画面です。

<div class="img-center" markdown="1">

![Qt Creator インポート]({{ site.baseurl }}/assets/images/documentation/QtCreator-import.png "Qt Creator インポート")

</div>

※ 画像をクリックすると拡大します。

次はデバッグのための実行時の設定です。<br>
tadpole コマンドの引数の末尾に、-eオプションとアプリケーションルートの絶対パスを指定します。-e オプションはDB環境を切り替えるのための設定でしたね。dev を指定することにします。

Linux の場合：<br>
次は、アプリケーションルートに /var/tmp/blogapp を指定したときの画面です。

<div class="img-center" markdown="1">

![Qt Creator runenv]({{ site.baseurl }}/assets/images/documentation/QtCreator-runenv(1).png "Qt Creator runenv")

</div>

WIndows の場合：<br>
上記の内容をビルド設定画面と実行時設定画面の２つで設定します。

ビルド設定の例：

<div class="img-center" markdown="1">

![ビルド設定の例]({{ site.baseurl }}/assets/images/documentation/QtCreator-build-settings-win.png "ビルド設定の例")

</div>

実行時の設定の例：

<div class="img-center" markdown="1">

![実行時の設定の例]({{ site.baseurl }}/assets/images/documentation/QtCreator-run-settings-win.png "実行時の設定の例")

</div>

以上で、設定は完了です。<br>
あとは、ソースコードにブレークポイントを追加し、Webブラウザからアクセスしてみてください。

ブレークポイントで処理が止まりましたか？