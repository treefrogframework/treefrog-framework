---
title: ダウンドード
page_id: "download.00"
---

## ダウンロード

### インストーラ for Windows

Qt6 向けのインストーラを提供しています。セットアップすれば、すぐに TreeFrog Framework 開発環境ができあがります。ソースコードからビルドしてインストールする必要がなくなりますので、手っ取り早く環境を作りたい方向けです。

<div class="table-div" markdown="1">

| バージョン                                      | ファイル                               |
|------------------------------------------------|---------------------------------------|
| 2.11.1 for Visual Studio 64bit (Qt6.9 or 6.8) | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-2.11.1-msvc_64-setup.exe](https://github.com/treefrogframework/treefrog-framework/releases/download/v2.11.1/treefrog-2.11.1-msvc_64-setup.exe) |

</div>

セットアップする前に、あらかじめ Qt6 をインストールしておく必要があります。

※ Linux, Mac OS X をお使いの方はソースコードからインストールしてください。

## ソースコード

ソースコードを tar.gz で固めたものを提供しています。インストール手順を参考にして、インストールしてください。

<div class="table-div" markdown="1">

| ソースパッケージ  | ファイル                         |
|-------------------|----------------------------------|
| バージョン 2.11.2 | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-framework-2.11.2.tar.gz](https://github.com/treefrogframework/treefrog-framework/archive/v2.11.2.tar.gz) |

</div>

 [以前のバージョンはこちら <i class="fa fa-angle-double-right" aria-hidden="true"></i>](https://github.com/treefrogframework/treefrog-framework/releases)

最新のソースコードは [GitHub](https://github.com/treefrogframework/) からどうぞ。

## Homebrew

Homebrewの[サイト](https://formulae.brew.sh/formula/treefrog)を参考にしてください。
macOS では、Homebrew からインストールすることができます。

```
 $ brew install qt qt-postgresql qt-mariadb
 $ brew install treefrog
```

Qt用のSQLドライバが正しくインストールされたならば、次のように表示されます。

```
 $ tspawn --show-drivers
 Available database drivers for Qt:
   QSQLITE
   QMARIADB
   QMYSQL
   QPSQL
```

`QMARIADB` や `QPSQL` が表示されない場合、ドライバが正しいディレクトリに格納されていません。次のコマンドを実行してドライバディレクトリのパスを確認し、そこにドライバを手動でコピーしてください。

```
(例)
 $ cd $(tspawn --show-driver-path)
 $ pwd
 (your_brew_path)/Cellar/qt/6.2.3_1/share/qt/plugins/sqldrivers
```

ドライバをコピーした後、ls コマンドで確認すると次のような結果になります。

```
$ ls $(tspawn --show-driver-path)
libqsqlite.dylib  libqsqlmysql.dylib  libqsqlpsql.dylib
```
