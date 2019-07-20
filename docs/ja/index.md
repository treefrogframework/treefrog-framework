---
title: ホーム
page_id: "home.00"
---

## <i class="fa fa-bolt" aria-hidden="true"></i> 小さくても パワフル そして 高性能

TreeFrog Framework は、C++によるフルスタックの高速Webアプリケーションフレームワークであり、HTTP はもちろん WebSocket プロトコルもサポートしています。

C++/Qt で作られたサーバサイドのフレームワークであるので、スクリプト言語のものより高速に動作することが可能です。アプリケーション開発では、MVC アーキテクチャのもと O/R マッパーやテンプレートの仕組みを提供し、「設定より規約」のポリシーでプログラミング言語C++でも高い生産性の実現を目指しています。


## <i class="fa fa-flag" aria-hidden="true"></i> 特徴

TreeFrog Framework には次のような特徴があります。

  1. 高パフォーマンス ： 高度に最適化されたアプリケーションサーバエンジン
  2. O/R マッピング ： 複雑で面倒なデータベースコーディングを隠蔽
  3. テンプレートシステム ： ERBライクなテンプレートエンジン
  4. 多くのDBに対応 ： MySQL, PostgreSQL, ODBC, SQLite, MongoDB, Redis, etc.
  5. クロスプラットフォーム ： Windows, macOS, Linuxで同じソースコードが動作
  6. WebSocket 対応 ： サーバと双方向通信が可能
  7. ジェネレータ ： 「足場」となるソースコードや Makefile を自動で生成
  8. 低リソース：  ラズベリーパイでも軽快に動作
  9. オープンソースソフトウェア ： New BSD License


## <i class="fa fa-comment" aria-hidden="true"></i> TreeFrog Framework という選択

Webアプリの開発において、開発効率と動作速度はトレードオフの関係があると言われますが、本当にそうなのでしょうか？

そんなことはありません。
フレームワークが便利な開発ツールと優れたライブラリを提供し、設定ファイルを極力減らす仕様とすることで、効率良く開発することができます。

近年、クラウドコンピューティングが台頭し、Web アプリの重要性は年々増しています。 スクリプト言語はコード量が増えるほど実行速度が落ちることは知られていますが、C++ はコード量が増えても実行速度は落ちませんし、メモリフットプリントが小さい上に最速で動作することが可能なのです。

スクリプト言語で稼働している複数のアプリケーションサーバを、パフォーマンスを低下させることなく１台に集約できます。
高い生産性と高速動作を両立した TreeFrog Framework をぜひお試し下さい。


## <i class="fa fa-bell" aria-hidden="true"></i> お知らせ

### 2019/7/20  TreeFrog Framework バージョン1.25.0 （安定版）リリース <span style="color: red;">New!</span>

  - Mongo Cドライバをv1.9.5に更新
  - LZ4圧縮アルゴリズムをv1.9.1に更新
  - セッションクッキーにドメイン値を設定できるよう修正
  - コンフィグをC++11からC++14に変更
  - その他不具合修正と改善

 [<i class="fas fa-download"></i> ダウンロードはこちらから](/ja/download/)

### 2019/4/29  TreeFrog Framework バージョン1.24.0 （安定版）リリース

  - LZ4圧縮アルゴリズムを使用するよう修正
  - Redisハッシュ型データ向けの関数を実装
  - 大文字を使った拡張子のファイルで戻されるコンテントタイプの不具合修正
  - バックスラッシュを含んだform-dataの解析の不具合修正
  - その他不具合修正と改善

### 2019/1/6  TreeFrog Framework バージョン1.23.0 （安定版）リリース

  - BEGEN/COMMIT/ROLLBACK 失敗時のファールセーフ追加
  - セッションストアの不具合修正
  - 例外クラスの微修正
  - クエリーログにログメッセージ追加
  - その他不具合修正

### 2018/6/10  TreeFrog Framework バージョン1.22.0 （安定版）リリース

 - WebアプリのCMakeビルドをサポート
 - ESMTPをサポートしていない古いタイプのSMTPサーバへの送信をサポート
 - tspawn.proファイルの不具合修正

### 2018/3/25  TreeFrog Framework バージョン1.21.0 （安定版）リリース

 - Mongoc共有ライブラリとのリンクオプションを追加
 - WindowsでHTTPヘッダの改行不具合の修正
 - アプリ更新時におけるリローディングの不具合修正
 - アクションが存在しないときに404コードを返すよう修正


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

ベンチマーク[(外部リンク)](https://www.techempower.com/benchmarks/#section=data-r16){:target="_blank"}
