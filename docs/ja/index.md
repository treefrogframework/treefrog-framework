---
title: ホーム
page_id: "home.00"
---

## 小さくて パワフル そして 高性能

TreeFrog Framework は、C++によるフルスタックの高速Webアプリケーションフレームワーク
であり、HTTP はもちろん WebSocket プロトコルもサポートしています。

C++/Qt で作られたサーバサイドのフレームワークであるので、スクリプト言語のものより高速に
動作することが可能です。アプリケーション開発では、MVC アーキテクチャのもと O/R マッパー
やテンプレートの仕組みを提供し、「設定より規約」のポリシーでプログラミング言語 C++でも
高い生産性の実現を目指しています。

## お知らせ

### 2017/7/1  TreeFrog Framework バージョン1.18.0 （安定版）リリース <span style="color: red;">New!</span>

 - MongoDBへのセッションストアを実装.
 - ファイルセッションストアの不具合修正.
 - コントローラジェネレータの不具合修正.
 - Windows サービスとして起動したときに引数解析の不具合修正.
 - DB接続オブジェクトがリークする可能性がある不具合修正.

 [ダウンロードはこちらから](download/)

### 2017/5/27  TreeFrog Framework バージョン1.17.0 （安定版）リリース

  - If-Modified-Sinceヘッダの比較ロジックの不具合修正
  - URLのトラバーサル不具合の修正
  - 静的ファイルへのルーティング設定を追加
  - バックグランドプロセスを実現するTBackgroundProcessクラスを追加
  - その他バグフィックス

### 2017/4/8  TreeFrog Framework バージョン1.16.0 （安定版）リリース

  - リッスンIPアドレスの設定を追加
  - DBオープン直後のSQL文実行の設定を追加
  - マルチフィールドのorder byに対応した関数を追加
  - GigHub Pages（日本語、英語）を追加
  - その他バグフィックス

### 2017/1/22  TreeFrog Framework バージョン1.15.0 （安定版）リリース

  - 'tDebug() << "foo" ' のようなデバッグ出力に対応
  - config-initializer関数を追加 （独自のコンフィグファイルを定義できるようになった）
  - TSqlORMapper が C++11 スタイルの for 文に対応
  - TFormValidator クラスの関数を修正
  - その他バグフィックス


## 募集中

協力者を募集しています！ [GitHub](https://github.com/treefrogframework/treefrog-framework){:target="_blank"}にてバグ報告やプルリクエストをお待ちしています。

 - 開発者、テスター、翻訳者

 当サイトは[GitHub Pages](https://pages.github.com/)で構築されているので、翻訳文をプルリクエストで送って頂くことができます。

## TreeFrog Framework という選択

Webアプリの開発において、開発効率と動作速度はトレードオフの関係があると言われますが、本当にそうなのでしょうか？

そんなことはありません。
フレームワークが便利な開発ツールと優れたライブラリを提供し、設定ファイルを極力減らす仕様とすることで、効率良く開発することができます。

近年、クラウドコンピューティングが台頭し、Web アプリの重要性は年々増しています。 スクリプト言語はコード量が増えるほど実行速度が落ちることは知られていますが、C++ はコード量が増えても実行速度は落ちませんし、メモリフットプリントが小さい上に最速で動作することが可能なのです。

スクリプト言語で稼働している複数のアプリケーションサーバを、パフォーマンスを低下させることなく１台に集約できます。
高い生産性と高速動作を両立した TreeFrog Framework をぜひお試し下さい。

## 特徴

TreeFrog Framework は次のような特徴を兼ね備えています。

 1. 高いパフォーマンス ： 高度に最適化されたアプリケーションサーバエンジン  [外部のベンチマークサイト](http://www.techempower.com/benchmarks/){:target="_blank"}
 2. O/R マッピング ： 複雑で面倒なデータベースアクセスを隠蔽
 3. テンプレートシステム ： テンプレートとプレゼンテーションロジックを完全に分離
 4. 多くのDBに対応 ： MySQL, PostgreSQL, ODBC, SQLite, Oracle, DB2, InterBase, MongoDB, Redis.
 5. クロスプラットフォーム ： Windows, Mac OS X, Linux など。同じコードが他のプラットフォームでも動作。
 6. WebSocket 対応 ： サーバと双方向通信を可能とします
 7. ジェネレータ ： 「足場」となるソースコードや Makefile を自動で生成
 8. 低リソース：  ラズベリーパイでも軽快に動作
 9. オープンソースソフトウェア ： New BSD License

## インフォメーション

 開発は主にGitHubで行われていますが[TreeFrogフォーラム](https://groups.google.com/forum/#!forum/treefrogframework){:target="_blank"}もあります。

ときどき つぶやきます [@TreeFrog_ja](https://twitter.com/TreeFrog_ja){:target="_blank"}

企業向け有償サポートは[イディ株式会社](http://www.ideeinc.co.jp/){:target="_blank"}にお問い合わせください。
