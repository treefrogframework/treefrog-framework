---
title: ビュー
page_id: "070.0"
---

## ビュー

Webアプリケーションにおけるビューの役割は、HTMLドキュメントを生成し、レスポンスとして返すことです。開発者は、コントローラから渡された変数を所定の部分に埋め込むためのHTMLドキュメントのテンプレート（ひな形）を作成します。また、条件分岐やループ処理を埋め込むことができます。

TreeFrog ではテンプレートシステムとして ERB を使うことができます。ERB は、Ruby にあるようにプログラムコードをテンプレートに書くことできるシステム（ライブラリ）のことです。仕組みが簡単で理解しやすいというメリットがある一方、Web デザインの確認や変更がしにくい面があります。

テンプレートシステムで作成されたコードをビルドすると、C++ に変換されたコードがコンパイルされ、１つの共有ライブラリ（ダイナミックリンクライブラリ）が作られます。C++ なので高速に動作します。

フロントエンドは JavaScript フレームワークに任せることで、バックエンドの処理は必要最低限の実装とすることができます。TreeFrog では、Vite + Vue のスキャフォールド（足場）を生成することができます。

テンプレートとプレゼンテーションロジックを分離したテンプレートシステムとして Otama が実装されていますが、非推奨となりました。

* [ERB テンプレートシステム の章へ >>](/ja/user-guide/view/erb.html){:target="_blank"}
* [Vite + Vue の章へ >>](/ja/user-guide/view/vite+vue.html){:target="_blank"}