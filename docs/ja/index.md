---
title: ホーム
page_id: "home.00"
---

## <i class="fa fa-bolt" aria-hidden="true"></i> 小さくても パワフル そして 高性能

TreeFrog Framework は、C++によるフルスタックの高速Webアプリケーションフレームワークであり、HTTP はもちろん WebSocket プロトコルもサポートしています。

C++/Qt で作られたサーバサイドのフレームワークであるので、スクリプト言語のものより高速に動作することが可能です。アプリケーション開発では、MVC アーキテクチャのもと O/R マッパーやテンプレートの仕組みを提供し、「設定より規約」のポリシーでプログラミング言語C++でも高い生産性の実現を目指しています。


## <i class="fa fa-flag" aria-hidden="true"></i> 特徴

TreeFrog Framework には次のような特徴があります。

  1. 高パフォーマンス ： 高度に最適化されたC++アプリケーションサーバエンジン
  2. O/R マッピング ： 複雑で面倒なデータベースコーディングを隠蔽
  3. テンプレートシステム ： ERBライクなテンプレートエンジン
  4. 多くのDBに対応 ： MySQL, MariaDB, PostgreSQL, ODBC, MongoDB, Redis, Memcached, etc.
  5. WebSocket 対応 ： サーバと双方向通信が可能
  6. ジェネレータ ： 「足場」となるソースコード, Makefile や vue.js テンプレートを生成
  7. 様々なレスポンスタイプに対応 ： JSON, XML, CBOR
  8. マルチプラットフォーム ： Windows, macOS, Linuxで同じソースコードが動作
  9. オープンソースソフトウェア ： New BSD License


## <i class="fa fa-comment" aria-hidden="true"></i> TreeFrog Framework という選択

Webアプリの開発において、開発効率と動作速度はトレードオフの関係があると言われますが、本当にそうなのでしょうか？

そんなことはありません。
フレームワークが便利な開発ツールと優れたライブラリを提供し、設定ファイルを極力減らす仕様とすることで、効率良く開発することができます。

近年、クラウドコンピューティングが台頭し、Web アプリの重要性は年々増しています。 スクリプト言語はコード量が増えるほど実行速度が落ちることは知られていますが、C++ はコード量が増えても実行速度は落ちませんし、メモリフットプリントが小さい上に最速で動作することが可能なのです。

スクリプト言語で稼働している複数のアプリケーションサーバを、パフォーマンスを低下させることなく１台に集約できます。
高い生産性と高速動作を両立した TreeFrog Framework をぜひお試し下さい。


## <i class="fa fa-bell" aria-hidden="true"></i> お知らせ

### 2025/9/27  TreeFrog Framework バージョン2.11.2 （安定版）リリース <span style="color: red;">New!</span>

 - 内部で使用している MongoDB C ドライバをバージョン 2.1.0 に更新
 - ロギング関数を std::format スタイルの書き方に対応

 [<i class="fas fa-download"></i> ダウンロードはこちらから](/ja/download/)

### 2025/7/5  TreeFrog Framework バージョン2.11.1 （安定版）リリース

 - tftest.hのコンパイルエラーを修正
 - テストマクロの更新

### 2025/5/5  TreeFrog Framework バージョン2.11.0 （安定版）リリース

 - Vite + Vue のスキャフォールディングサポート（実験的）  [Vite+Vue](/ja/user-guide/view/vite+vue.html)参照
 - macOS の TSharedMemory クラスの不具合修正

### 2024/11/30  TreeFrog Framework バージョン2.10.0 （安定版）リリース

 - std::format スタイルのログ出力に対応, Tf::error(), Tf::warn(), Tf::info()など
 - tsharedmemory初期化の不具合修正
 - このバージョン以降 Qt6 のみサポート (Qt5は非サポート)

### 2024/6/15  TreeFrog Framework バージョン2.9.0 （安定版）リリース

 - ステータスコードに関するアクセスログ出力の不具合修正
 - TSqlObjectで値がQString()である場合にNULLを設定するように修正
 - TAbstractModel::setProperties(const QJsonObject &properties) 関数を追加
 - Mongoc driver を v1.26.2 に更新
 - glog を v0.7.0 に更新

### 2023/12/10  TreeFrog Framework バージョン2.8.0 （安定版）リリース

  - PostgreSQL と MySQL のプリペアドステートメントを対応
  - Emscripten でのコンパイルエラーを解消

### 2023/3/26  TreeFrog Framework バージョン2.7.1 （安定版）リリース

  - 共有メモリKVSをオープンする際の不具合修正
  - アクションを呼び出せない場合にNotFoundを返すように修正

### 2023/2/25  TreeFrog Framework バージョン2.7.0 （安定版）リリース

  - パケット受信時にスレッド衝突の可能性がある不具合を修正
  - ハッシュアルゴリズムをSHA3-HMACへ変更
  - セッションストアにMemcachedを追加
  - TSharedMemoryAllocatorのmallocアルゴリズムを更新
  - システムロガーを更新
  - データベース接続プーリングのパフォーマンス改善

 [<i class="fa fa-list" aria-hidden="true"></i> 全ての変更履歴](https://github.com/treefrogframework/treefrog-framework/blob/master/CHANGELOG.md)


## <i class="fa fa-user" aria-hidden="true"></i> 募集中

協力者を募集しています！ [GitHub](https://github.com/treefrogframework/treefrog-framework){:target="_blank"}にてバグ報告やプルリクエストをお待ちしています。

 - 開発者、テスター、翻訳者

 当サイトは[GitHub Pages](https://pages.github.com/)で構築されているので、翻訳文をプルリクエストで送って頂くことができます。


## <i class="fa fa-info-circle" aria-hidden="true"></i> インフォメーション

 開発は主にGitHubで行われていますが[TreeFrogフォーラム](https://groups.google.com/forum/#!forum/treefrogframework){:target="_blank"}もあります。

ときどき つぶやきます [@TreeFrog_ja](https://twitter.com/TreeFrog_ja){:target="_blank"}

企業向け有償サポートは[イディ株式会社](http://www.ideeinc.co.jp/){:target="_blank"}にお問い合わせください。

Dockerイメージ[(外部リンク)](https://hub.docker.com/r/treefrogframework/treefrog/){:target="_blank"}

ベンチマーク[(外部リンク)](https://www.techempower.com/benchmarks/){:target="_blank"}
