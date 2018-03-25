---
title: ブログ
page_id: "blog.00"
---

## Web Framework Benchmarks Round 15

<https://www.techempower.com/benchmarks/#section=data-r15>{:target="_blank"}


## Web Framework Benchmarks Round 13

<https://www.techempower.com/benchmarks/#section=data-r13>{:target="_blank"}

MongoDB の設定を追加してプルリクエストを送ったところマージされ、ベンチテストされてました。

ランキングで、treefrog が何を指しているかわからないので補足：

  - treefrog : TreeFrog (MPM:thread) + MySQL
  - treefrog-postgres : TreeFrog (MPM:thread) + PostgreSQL
  - treefrog-mongo : TreeFrog (MPM:thread) + MongoDB
  - thread-hybrid : treefrog (MPM:hybrid) + MySQL


## C++インタプリタ – cpi

C++１１のコードをちょっと確認したいときに役立つインタプリタを作ってみました。

 <http://treefrogframework.github.io/cpi/>


## WebSocket でチャット

WebSocket でチャットを作ってみました。 遊んでみてください。そのうち作り方をアップしようと思います。

 <http://chatsample.treefrogframework.org>

サーバはもちろん TreeFrog で実装されています。


## Webフレームワークベンチマーク Round 9

Webフレームワークベンチマーク Round 9 が公開されています。
TreeFrog Framework のコードは私がプルし、そのベンチマーク結果がありますので参考になさってください。

<http://www.techempower.com/benchmarks/>

MVC フレームワークの中では、トップグループに入るパフォーマンスが示されています。


## Webフレームワークのベンチマーク

Webフレームワークのスループットを比較したサイトがあります。

[Web Framework Benchmarks](http://www.techempower.com/benchmarks/)

有名なものからあまり知られていないものまで、多くのフレームワークが含まれています。
TreeFrog Framework も登録してみました。

内容として、**HTTP接続は Keep-Alive のまま、単純な１種類のリクエストを何回処理できるか**というものです。

実際とはかけ離れたテストであると言えますが、１つの参考にはなるかもしれません。単純なテストのベンチマークであるので、MVCなフレームワークには全体的にやや不利な結果がでているようです。

実際の運用では、そんな単純な処理ばかりではありません。
Webサーバ（APサーバ）では、HTTPの接続・切断が頻繁に繰り返されますし、時間の掛かるものから掛からないものまで様々なリクエストを同時に処理します。

そんなベンチマークをしてみると、また違う結果になるかもしれませんね。

Webサーバ（APサーバ）としては、リクエストの受信からレスポンスの送信までを、スレッドを介さずに同期的な関数だけで実装すれば非常に単純でスループットも出せるでしょう。
今回のベンチマークでも、良い結果がだせそうです。

ただ、この実装だと、あるユーザへ返すレスポンスが、他のユーザの時間の掛かるリクエストに引きづられて遅くなってしまうでしょう。これはAPサーバの実装として好ましくないと思います。

ちなみに、TreeFrog Framework は、リクエスト毎にマルチスレッド化しているので、他のリクエストに引きづられてしまうことはありません。


## サンプルアプリの blogapp を公開してみました

チュートリアルで作ったやつです。ここ↓にありますので、 遊んでみて。

<http://blogapp.treefrogframework.org/Blog>

適当に停止します。
