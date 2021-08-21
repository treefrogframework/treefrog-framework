---
title: デプロイ
page_id: "130.0"
---

## デプロイ

開発したアプリケーションを本番環境（あるいはテスト環境）に配置し動作させます。

本番環境でソースコードをビルドできるのならばそれが一番手軽ですが、一般には本番環境とビルドマシンは別になるでしょう。ここでは、ビルド用として本番環境と同じOS／ライブラリを組み込んだコンピュータを用意し、そこでリリース用バイナリをビルドします。作成されたバイナリと関連するファイルをアーカイブに固めて、本番環境へ移すことにします。

### リリースモードでのビルド

ソースコードをリリースモードでビルドするために、アプリケーションルートで次のコマンドを実行します。環境に最適化されたバイナリが生成されます。

```
 $ qmake -r "CONFIG+=release"
 $ make clean
 $ make
```

### 本番環境へのデプロイ

まず、本番環境用の設定内容を確認します。database.ini 設定ファイルの [product] セクションにあるユーザ名／パスワード、さらに application.ini ファイルにあるリッスンポートの番号を確認しておきましょう。環境に合わせて、修正してください。

アプリケーションが正常に動作するために必要なファイルをまとめます。次のディレクトリにあるファイルおよびサブディレクトリを全てアーカイブします。

* config
* db      ← sqlite を使用しない場合は不要
* lib
* plugin
* public
* sql

tar コマンドの例：

```
 $ tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
```

※ tarファイル名は適宜変えてください。

次に、本番環境の構築です。データベースシステムの構築・設定、TreeFrog Framework / Qt のインストールはあらかじめ完了しているとします。

tar ファイルを本番環境へコピーします。コピーしたら、ディレクトリを作って展開します。

```
 $ mkdir app_name
 $ cd app_name
 $ tar xvfz (ディレクトリ名)/app.tar.gz
```

次のコマンドで起動します。コマンドのオプションにはアプリケーションルートディレクトリを**絶対パス**で指定します。

```
 $ sudo treefrog -d  [application_root_path]
```

ディストリビューションによって、80番ポートをオープンする場合はルート権限は必要な場合があります。この例では、sudo コマンドで起動しています。

また、Linux では、init.d スクリプトを作ってアプリを自動的に起動させることができます。Window では、スタートアップに登録することで可能です。OSのブート時に自動でサービスを起動させる方法はネット上にたくさん記事があるので、ここでは割愛します。

停止コマンドは次でしたね。

```
 $ sudo treefrog -k stop  [application_root_path]
```

### CIツールを使ったビルド
リリース用バイナリのビルド、または本番に近い環境でのテストにはCIツールが有効な手段の一つです。
下に例を示します。
```yaml
# GitHub Action
# .github/workflows/c-cpp.yml
name: C/C++ CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:

    runs-on: ubuntu-18.04

    steps:
    - uses: actions/checkout@v2

    # build treefrog
    - name: dependency install
      run: sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev qtdeclarative5-dev qtbase5-dev-tools gcc g++ make cmake
    - name: dependency db install
      run: sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1 libmongoc-dev libbson-dev
    - name: download treefrog source archive
      run: wget https://github.com/treefrogframework/treefrog-framework/archive/v1.30.0.tar.gz
    - name: expand treefrog archive 
      run: tar zxvf v*.*.*.tar.gz
    - name: configure treefrog
      run: cd treefrog-framework-*.* && ./configure --prefix=/usr/local
    - name: make treefrog
      run: cd treefrog-framework-*.* && make -j4 -C src
    - name: make install treefrog
      run: cd treefrog-framework-*.* && sudo make install -C src
    - name: make treefrog tools
      run: cd treefrog-framework-*.* && make -j4 -C tools
    - name: make install treefrog tools
      run: cd treefrog-framework-*.* && sudo make install -C tools
    - name: update share library dependency info
      run: sudo ldconfig
    - name: check treefrog version
      run: treefrog -v
    
    # build project files
    - name: execute qmake.
      run: qmake -r "CONFIG+=release"
    - name: makeing directory for build
      run: mkdir build
    - name: cmake 
      run: cd build && cmake ../
    - name: execute make
      run: cd build && make

    # package project files.
    - name: compressive archive
      run: tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
    - name: staging
      run: mkdir staging && mv app.tar.gz staging/
    - uses: actions/upload-artifact@v2
      with:
        name: Package
        path: staging
```