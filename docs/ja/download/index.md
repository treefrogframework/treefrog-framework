---
title: ダウンドード
page_id: "download.00"
---

## ダウンロード

### インストーラ for Windows

Qt5 向けのインストーラを提供しています。セットアップすれば、すぐに TreeFrog Framework 開発環境ができあがります。ソースコードからビルドしてインストールする必要がなくなりますので、手っ取り早く環境を作りたい方向けです。

<div class="table-div" markdown="1">

| バージョン                                       | ファイル                                  |
|------------------------------------------------|---------------------------------------|
| 2.0.2 for Visual Studio 2019 64bit (Qt6.2 or 6.1) | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-2.0.2-msvc_64-setup.exe](https://github.com/treefrogframework/treefrog-framework/releases/download/v2.0.2/treefrog-2.0.2-msvc_64-setup.exe) |

</div>

セットアップする前に、あらかじめ Qt6 をインストールしておく必要があります。

※ Linux, Mac OS X をお使いの方はソースコードからインストールしてください。

## ソースコード

ソースコードを tar.gz で固めたものを提供しています。インストール手順を参考にして、インストールしてください。

<div class="table-div" markdown="1">

| ソースパッケージ  | ファイル                         |
|-------------------|----------------------------------|
| バージョン 2.0.2 | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-framework-2.0.2.tar.gz](https://github.com/treefrogframework/treefrog-framework/archive/v2.0.2.tar.gz) |

</div>

 [以前のバージョンはこちら <i class="fa fa-angle-double-right" aria-hidden="true"></i>](https://github.com/treefrogframework/treefrog-framework/releases)

最新のソースコードは [GitHub](https://github.com/treefrogframework/) からどうぞ。

## Homebrew

macOS では、Homebrew からインストールすることができます。

```
 $ brew install mysql-connector-c
 $ brew install --with-mysql --with-postgresql treefrog
```

Qt をコンパイルするので、かなり時間がかかります。

もし、SQLite にだけアクセスする（MySQL にも PostgreSQL にもアクセスしない）ならば、 "brew install treefrog" だけでインストールできます。
