---
title: プラグイン
page_id: "080.060"
---

## プラグイン

TreeFrog におけるプラグインとは、機能を拡張するために追加される動的ライブラリ（共有ライブラリ、DLL）のことを指します。TreeFrog には Qt によるプラグインの仕組みが備わっているので、標準機能で不十分な場合はプラグインを作成することができます。現時点、プラグインの作成可能なカテゴリは次のとおりです。

* ロガープラグイン    -  ログ出力
* セッションストアプラグイン   -  セッションの保存と読込

※ まだ追加する予定。

## プラグインの作成

プラグインの作り方は、まさに Qt のプラグインの作り方と全く同じです。実例を示すために、ロガープラグインを作成してみましょう。まず、作業ディレクトリを plugin ディレクトリにつくります。

```
 > cd plugin
 > mkdir sample
```

※ 作業ディレクトリはどこでも構わないのですが、パスの解決を考えるとここがいいのです。

これから作成するプラグインのインターフェースとなるクラス（この例では TLoggerPlugin） を継承し、いくつかの仮想関数をオーバライドしたプラグインラッパクラスを作成します。

```c++
sampleplugin.h
---
class SamplePlugin : public TLoggerPlugin
{
    QStringList keys() const;
    TLogger *create(const QString &key);
};
```

keys() メソッドでは、プラグインでサポートする機能を指すキーとなる文字列をリストで返します。create() メソッドでは、キーに対応するロガーのインスタンスを生成し、そのポインタを返却するように実装します。

このプラグインを登録するために、ソースファイルで QtPlugin をインクルードした上で、末尾に次のようなマクロをいれてください。第一引数はプラグインの名前で好きな文字列を指定できます。第二引数はプラグインクラス名です。

```c++
sampleplugin.cpp
---
#include <QtPlugin>
  :
  :
Q_EXPORT_PLUGIN2(samplePlugin, SamplePlugin)
```

次に、プラグインとして拡張する機能（クラス）を作ります。ここではログを出力するロガーを作ります。上と同様に、ロガーのインターフェースとなる TLogger クラスを継承して、いくつかの仮想関数をオーバライドします。

```c++
samplelogger.h
---
class SampleLogger : public TLogger
{
public:
    QString key() const { return "Sample"; }
    bool isMultiProcessSafe() const;
    bool open();
    void close();
    bool isOpen() const;
    void log(const TLog &log);
      :
};
```

次は、プロジェクトファイル(.pro)です。CONFIG パラメータに "plugin" を忘れずに追加します。

```
TARGET = sampleplugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = ..
include(../../appbase.pri)
HEADERS = sampleplugin.h \
          samplelogger.h
SOURCES = sampleplugin.cpp \
          samplelogger.cpp
```

※ include 関数で、appbase.pri ファイルをインクルードするのが<span style="color: red">重要</span>。

こうしてからビルドすると、動的ロード可能なプラグインが作成されます。プラグインは**必ず** plugin ディレクトリに保存してください。アプリケーションサーバ（APサーバ）は、このディレクトリにあるプラグインを読み込みます。<br>
[プラグインシステム](http://doc.qt.io/qt-5/plugins-howto.html){:target="_blank"}の詳細は Qt ドキュメントをご覧ください。

## ロガープラグイン

標準搭載された FileLogger はファイルへログを出力する基本のロガーですが、要件によっては不十分かもしれません。ログはデータベースに保存したいとか、あるいはログファイルはローテーションしながら残したいとか、など。こういった場合、プラグインの仕組みを使って機能を拡張することができます。

上記で説明したように、ロガープラグインを作成し plugin ディレクトリに置きます。さらに、アプリケーションサーバにロードさせるために logger.ini ファイルへ設定情報を書きます。使用するロガーのキーを Loggers パラメータにスペース区切りで並べます。次のようになります。

```
 Loggers=FileLogger Sample
```

こうすることで、アプリケーションサーバの起動時にプラグインがロードされるようになります。

繰り返しになりますが、ロガーのためのプラグインインターフェースは次のクラスです。

* プラグインインターフェース ： TLoggerPlugin
* ロガーインターフェース ： TLogger

### ロガーのメソッドについて

ロガーを実装するため、TLogger クラスにある次のメソッドをオーバライドします。

* key() ： ロガーの名前を返す。
* open() ： ログのオープン。プラグインロード直後に呼ばれる。
* close() ： ログのクローズ。
* log() ： ログを出力するメソッド。このメソッドは複数のスレッドから呼ばれるので、必ず**スレッドセーフ**にします。
* isMultiProcessSafe() ： open() メソッドを呼んだままマルチプロセスでログを出力しても安全かどうかを示すもので、安全である場合 true を返します。そうでない場合 false を返します。

isMultiProcessSafe()  メソッドについて、これが false を返し（安全でない）、かつアプリケーションサーバがマルチプロセスモードで起動している場合（[MPM]({{ site.baseurl }}/ja/user-guide/performance/index.html){:target="_blank"} が prefork の場合）、ログ出力の前後で毎回 open / close メソッドを呼ぶようになります（オーバヘッドが大きくなる）。ちなみに、システムはこの前後でセマフォによるロック／アンロックをかけているので、競合は発生しません。また true を返す場合、システムは open() メソッドを最初の１度しか呼ばなくなります。

## セッションストアプラグイン

TreeFrog に標準搭載されたセッションストアは次のとおりです。

* クッキーセッション ：  クッキーに保存
* DBセッション ：  DB に保存。専用のテーブルを作る必要あり。
* ファイルセッション ：  ファイルに保存

これらでは不十分であれば、次のインターフェースクラスを継承してプラグインを作成することができます。

* プラグインインターフェース ： TSessionStorePlugin
* セッションストアインターフェース ： TSessionStore

上記と同様に、これらのクラスを継承して仮想関数をオーバライドして、プラグインを作成します。その後、このプラグインをロードさせるため application.ini ファイルの Session.StoreType パラメータにキーを１つだけ設定します（複数指定は不可）。デフォルトは cookie が設定されています。