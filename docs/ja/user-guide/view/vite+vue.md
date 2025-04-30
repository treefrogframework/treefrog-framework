---
title: Vite + Vue
page_id: "070.015"
---

## Vite + Vue

Vite はフロントエンド開発のための高速ビルドツールであり、Vue.js の作者によって開発されました。ただし、Vite 自体はビルドツールであるので、Vue.js や React などのフロントエンドフレームワークと組み合わせて使用することになります。

フロントエンドフレームワークを使用する場合、Webアプリケーションの構成は、一般に SPA（Single Page Application）や MPA（Multi Page Application）スタイルで設計されます。

TreeFrog では、MPA スタイルの Vite+Vue 向けスキャフォールド（開発用の足場）を作成することが可能となっています。この時のフロントエンドとバックエンドの役割分担は次の通りです。

<div class="table-div" markdown="1">

| サイド           |   役割          |
| --------------- | --------------- |
| フロントエンド   | UI、ページレイアウト (HTML/CSS)、アニメーション |
| バックエンド     | リクエスト受取、ページ遷移、DBアクセス、ビジネスロジック |

</div><br>

ページごとに独立した構成となる MPA スタイルは比較的シンプルであるので、中小規模のサイトに適していると言えるでしょう。


## Vite + Vue 向けのアプリケーションスケルトンを生成

まず[チュートリアル](/ja/user-guide/tutorial/index.html){:target="_blank"}のページを読み、大まかな流れを理解しておいてください。次では、チュートリアルと同様に blogapp という名で Vite+Vue 向けのスケルトンを作ります。下記では Yarn コマンドを使用するので予めインストールしておいてください。

まず、アプリケーション生成時に`--template`オプションを指定します。フロントエンドのソースコードは _frontend_ ディレクトリに置きます。

```
 $ tspawn new blogapp --template vite+vue
  created   blogapp
  created   blogapp/controllers
  created   blogapp/models
  created   blogapp/models/sqlobjects
  created   blogapp/views
  created   blogapp/views/layouts
  created   blogapp/views/mailer
  created   blogapp/views/partial
   :

 $ cd blogapp
 $ yarn create vite frontend --template vue
    :
    :  （frontendディレクトリにViteとVueのコードが作られる）
    :
  
 $ cd frontend
 $ yarn
    :
    :  （必要なモジュールがインストールされる）
    :
```

`yarn create vite`コマンドで作成されたファイルの中は不要なコードがあるので、適宜変更または削除して構いません。

次にスキャフォールドを作ります。その前に、チュートリアルを参考にしてデータベースの設定（config/database.ini）やテーブルの作成は済ませておいてください。

```
 $ cd ..
 $ tspawn scaffold blog
  driverType: QSQLITE
  databaseName: blogdb
  hostName:
  Database open successfully
  created   models/sqlobjects/blogobject.h
  created   models/objects/blog.h
  created   models/objects/blog.cpp
  created   models/models.pro
  created   controllers/blogcontroller.h
  created   controllers/blogcontroller.cpp
  created   controllers/controllers.pro
  created   models/blogservice.h
  created   models/blogservice.cpp
  created   models/models.pro
  created   views/blog/index.erb
  created   views/blog/show.erb
  created   views/blog/create.erb
  created   views/blog/save.erb
  created   frontend/src/components/BlogIndex.vue
  created   frontend/src/components/BlogShow.vue
  created   frontend/src/components/BlogCreate.vue
  created   frontend/src/components/BlogSave.vue
```

ERBファイルとともに、Vueファイルも作成されました。index.erb を見てみましょう。
```
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Blog: index</title>
<% if (databaseEnvironment() == "dev") { %>
  <script type="module" src="http://localhost:5173/src/main.js"></script>
<% } else { %>
  <%== viteScriptTag("src/main.js", a("type", "module")) %>
<% } %>
  <meta name="authenticity_token" content="<%= authenticityToken() %>">
</head>
<body>
<div data-vue-component="BlogIndex"></div>
<script id="BlogIndex-props" type="application/json"><%==$ props %></script>
</body>
</html>
```

この例では、エントリーポイントは main.jsであり、フロントエンドアプリケーションは `<div data-vue-component="Foo"></div>` の部分に差し込まれます。データは `<script id="Foo-props" type="application/json"> ... </script>` に JSON 形式で書き込んでいます。


JSON データを渡してフロントエンドアプリケーションを起動するために、src/main.js ファイルを次のように書き換えます。手動で編集してください。
```
import { createApp, defineAsyncComponent } from 'vue'

// Import components
const modules = import.meta.glob('./components/*.vue')

// Execute createApp()
document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('[data-vue-component]').forEach(element => {
    const name = element.dataset.vueComponent
    const mod = modules[`./components/${name}.vue`]
    if (mod) {
      const rawProps = document.getElementById(name + '-props')?.textContent?.trim()
      const props = JSON.parse(rawProps || '{}')
      createApp(defineAsyncComponent(mod), props).mount(element)
    } else {
      console.error(`Component not found: ${name}.vue`)
    }
  })
})
```

このように、バックエンドからフロントエンドへ渡すデータは、HTML 上に JSON 形式で直接埋め込み、createApp 関数でフロントエンドアプリケーションを起動する際に引数として渡します。逆に、フロントエンドからバックエンドにデータを渡し方は一般的なHTMLのケースと同じであり、フォームデータを POST することで行います。MPA 構成では、多くのページにおいて Web API を使わずにデータの受け渡しが可能です。

## 開発モードで起動

チュートリアルにある手順で C++ コードをビルドしてください。ビルド完了後、開発モードでバックエンドサーバを起動します。

次の例の`-r`は自動読み込みオプションであり、C++を修正してビルドが完了すると、バックエンドアプリケーションが自動で読み込まれます。バックエンドサーバを再起動する必要がありません。また、`-e dev`オプションを指定することによって database.ini の dev エントリーの設定が使用されています。
```
 $ treefrog -e dev -r
  (止める場合は Ctrl + c )
```

別のターミナルで、Vite開発サーバを起動します。そうすることで、フロントエンドの修正を自動で即座に反映させることができます（HMR：Hot Module Replacement）。ブラウザで再読み込みする必要もなく、とても便利です。但し、Vite 開発サーバは開発時のみに使用するものであり、正式リリース版では必要ありません。
```
 $ cd frontend
 $ yarn dev
```

[http://localhost:8800/blog](http://localhost:8800/blog){:target="_blank"} にアクセスすると、ひととおりの CRUD は動作していると思います。Vite の機能によって、フロントエンドを修正し保存するとその内容が即座に反映されます。ブラウザでページを再読み込みする必要がないため、快適に開発が進められるでしょう。


バックエンドサーバは`-r`オプションを指定したことでビルドが完了すると自動的に再読み込みされますが、ブラウザで再読み込みしないと修正後のページを確認できません。HMR はフロントエンド向けの機能だからです。

これでは不便なので、バックエンドの修正も即座に反映させるために、次のエントリーを vite.config.ts の _plugins_ に追加します。このサンプルでは`.so`ファイルを監視し、変更があったらフルリロードさせています。macOS の場合は`.dylib`に変更するとよいでしょう（10行目付近）。
```
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { watch } from 'fs'   // <-- ここ追加

export default defineConfig({
  plugins: [
    vue(),
    // --------------ここから
    {
      name: 'treefrog-watcher',
      configureServer(server) {
        let timer = null;
        const watcher = watch('../lib', (e, f) => {
          if (f && f.endsWith('.so') && !timer) {
            timer = setTimeout(() => {
              console.log(`${new Date().toTimeString().slice(0, 8)} hmr full reload: ${f}`);
              server.ws.send({type: 'full-reload', path: '*'});
              timer = null;
            }, 1000);  // trigger after 1000s
          }
        });
      }
    },
    // --------------ここまで
  ],
  :
  :
```

これによって、バックエンドは make コマンドでビルドが正常に完了すると、約1秒後にブラウザ画面に反映されます。サーバがバックエンドアプリケーションの読み込みを完了するのに少し時間がかかるため、この例ではページフルリロードのトリガーはその1秒後に発火させています。


## リリース版起動

リリース版では Vueファイルは全て JavaScript ファイルに変換し、 _public_ ディレクトリに置く必要があります。次の _build_ エントリを vite.config.ts に追加します。バックエンドサーバが JavaScript のファイル名を識別する必要があるため、manifest ファイルを生成するのは必須です。

```
export default defineConfig({
    :
    :
  // --------------ここから
  build: {
    manifest: true,
    outDir: '../public',
    rollupOptions: {
      input: 'src/main.js'
    }
  },
  // --------------ここまで
})
```

フロントエンドのコードをビルドして、JavaScript に変換します。

```
 $ cd frontend
 $ yarn build
```

データベース情報を database.ini の _product_ エントリーに設定しておきます。その後、正式版で動作を確認しましょう。`-d`オプションを指定すると、バックエンドサーバをバックグラウンドで起動できます。Vite 開発サーバは必要ないので停止しておきます（Ctrl + c）。
```
 $ cd blogapp
 $ treefrog -p 80 -e product -d

 （停止する場合）
 $ treefrog -k stop
```

※ OS によって80番ポートで起動するためにはルート権限が必要な場合があります。その場合は sudo コマンドとともに実行します。同様に、停止する場合も sudo コマンドとともに実行します。

[http://localhost/blog](http://localhost/blog){:target="_blank"} にアクセスすると、 _public/assets_ ディレクトリにある JavaScript が使用され、開発モードと同じ動作をしていることを確認できます。

ちなみに、リリース版では HMR は作動しません。
