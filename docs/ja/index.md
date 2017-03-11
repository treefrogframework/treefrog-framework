---
title: お知らせ
---

## *小さくて パワフル そして 高性能*

TreeFrog Framework は、C++によるフルスタックの高速Webアプリケーションフレームワークであり、HTTP はもちろん WebSocket プロトコルもサポートしています。

C++/Qt で作られたサーバサイドのフレームワークであるので、スクリプト言語のものより高速に動作することが可能です。アプリケーション開発では、MVC アーキテクチャのもと O/R マッパーやテンプレートの仕組みを提供し、「設定より規約」のポリシーでプログラミング言語 C++ でも高い生産性の実現を目指しています。

## お知らせ

Jan 22, 2017, <span style="color: rgb(62, 109, 192); ">TreeFrog Framework バージョン1.15.0 （安定版）リリース</span>&nbsp;&nbsp;<span style="color: red; ">New!</span>

* 変更履歴：
  - 'tDebug() << "foo" ' のようなデバッグ出力に対応
  - config-initializer関数を追加 （独自のコンフィグファイルを定義できるようになった）
  - TSqlORMapper が C++11 スタイルの for 文に対応
  - TFormValidator クラスの関数を修正
  - その他バグフィックス
  
[ダウンロードはこちらから >>](http://www.treefrogframework.org/download){:target="_blank"}  
  
2016/12/5, <span style="color: rgb(62, 109, 192); ">TreeFrog Framework バージョン1.14.0 （安定版）リリース</span>

* 変更履歴：
  - thread_local の代わりにQThreadStorageを使うよう修正
  - スキャフォールディングでより良いコードを出力するよう修正
  - ERB: #partial キーワードを追加
  - Windowsにおける renderPartial() の不具合修正
  - PostgreSQL にセッションオブジェクト保存に関する不具合修正
  - パフォーマンス改善
  - その他バグフィックス
          
2016/10/17, <span style="color: rgb(62, 109, 192); ">TreeFrog Framework バージョン1.13.0 （安定版）リリース</span>

* 変更履歴：
  - ロックフリーなハザードポインタを実装
  - MSVC2015でのコンパイルエラーを修正
  - TAtomic クラスとTAtomicPtr クラスを追加
  - パフォーマンスチューニング
  - その他バグフィックス
  
協力者を<span style="color: red">募集</span>しています！[ML.](https://lists.sourceforge.net/lists/listinfo/treefrog-user){:target="_blank"}か、直接かメールください。 バグ報告歓迎です！
 ・ 本サイトを英訳してくれる方、どなたかお願いします m(_ _)m
 ・ 開発者、テスター、ドキュメント作成

ときどき つぶやきます [@TreeFrog_ja](https://twitter.com/TreeFrog_ja){:target="_blank"} 
 
企業向け有償サポートは[イディ株式会社](http://www.ideeinc.co.jp/){:target="_blank"} にお問い合わせください。