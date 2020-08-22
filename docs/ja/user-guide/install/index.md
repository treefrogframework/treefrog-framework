---
title: インストール
page_id: "020.0"
---

## インストール

あらかじめ Qt ライブラリをインストールしておいてください。

Windows や macOS の場合は、 Qt の[サイト](https://www.qt.io/download/){:target="_blank"}からダウンロードしてインストールしてください。
Linux の場合は、ディストリビューションで用意されているパッケージを使用することができます。

Ubuntu の例：

```
 $ sudo apt-get install -y qt5-default qt5-qmake libqt5sql5-mysql libqt5sql5-psql
 libqt5sql5-odbc libqt5sql5-sqlite libqt5core5a libqt5qml5 libqt5xml5 qtbase5-dev
 qtdeclarative5-dev qtbase5-dev-tools gcc g++ make cmake
```

DBクライアントライブラリもインストールします。

```
 $ sudo apt-get install -y libmysqlclient-dev libpq5 libodbc1 libmongoc-dev libbson-dev
```

## インストール手順

1. [ダウンロード](http://www.treefrogframework.org/ja/download/){:target="_blank"}したファイルを解凍します。下記の x.x.x にはバージョンが入りますので、適宜読み替えてください。

   ```
   $ tar xvfz treefrog-x.x.x.tar.gz
   ```

2. 解凍したディレクトリでビルドします。

   Windows の場合：
   リリース版とデバッグ版の２つのタイプのバイナリを作成してください。  Qt Command Prompt を起動し、次のコマンドでビルドします。configureバッチを２度実行します。

   ```
  > cd treefrog-x.x.x
  > configure --enable-debug
  > cd src
  > nmake install
  > cd ..\tools
  > nmake install
  > cd ..
  > configure
  > cd src
  > nmake install
  > cd ..\tools
  > nmake install
   ```

   Linux, macOS など UNIX 系 OS の場合：  コマンドラインから次を入力します。

   ```
  $ cd treefrog-x.x.x
  $ ./configure
  $ cd src
  $ make
  $ sudo make install
  $ cd ../tools
  $ make
  $ sudo make install
   ```

   ※ もしTreeFrog Framework 自体をデバッグするには、configureスクリプトにオプションをつけてください。 './configure --enable-debug' を実行し、あと同様のコマンドを実行します。

   Linux の場合には共有ライブラリの依存関係情報を更新する

   ```
  $ sudo ldconfig
   ```

3. TreeFrog Command Prompt  のショートカットを作成 する（Window のみ）。

   ショートカットを作成するフォルダで右クリックし、「新規作成」-「ショートカット」をクリックします。
   リンク先は次を設定します。

   ```
C:\Windows\System32\cmd.exe /K  C:\TreeFrog\x.x.x\bin\tfenv.bat
   ```

  （x.x.x はバージョン）

   <div class="img-center" markdown="1">

   ![ショートカット]({{ site.baseurl }}/assets/images/documentation/shortcut.png "ショートカット")

   </div>

   名前は 'TreeFrog Command Prompt'とします。

   <div class="img-center" markdown="1">

   ![ショートカットの名前]({{ site.baseurl }}/assets/images/documentation/shortcut-name.png "ショートカットの名前")

   </div>

   Windowsでは、あらかじめ環境変数が設定された TreeFrog Command Prompt  を使って TreeFrog のアプリケーションを開発していきます。

## Configure option

オプションを指定することで、お使いの環境に合わせてカスタマイズすることができます。

Windows で指定可能なオプション：

```
 > configure --help
 Usage: configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [C:\TreeFrog\x.x.x]
```

Linux, UNIX系OSで指定可能なオプション：

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library
   --spec=SPEC         use SPEC as QMAKESPEC

 Installation directories:
   --prefix=PREFIX     install files in PREFIX [/usr]

 Fine tuning of the installation directories:
   --bindir=DIR        user executables [/usr/bin]
   --libdir=DIR        object code libraries [/usr/lib]
   --includedir=DIR    C header files [/usr/include/treefrog]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```

macOS で指定できるオプション：

```
 $ ./configure --help
 Usage: ./configure [OPTION]... [VAR=VALUE]...
 Configuration:
   -h, --help          display this help and exit
   --enable-debug      compile with debugging information
   --enable-gui-mod    compile and link with QtGui module
   --enable-mongo      compile with MongoDB driver library

 Fine tuning of the installation directories:
   --framework=PREFIX  install framework files in PREFIX [/Library/Frameworks]
   --bindir=DIR        user executables [/usr/bin]
   --datadir=DIR       read-only architecture-independent data [/usr/share/treefrog]
```
