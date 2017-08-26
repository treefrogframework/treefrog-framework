---
title: パフォーマンス
page_id: "160.0"
---

## パフォーマンス

### アプリケーションサーバの構成

TreeFrog のアプリケーションサーバ（APサーバ）は、HTTPリクエストを処理する複数のサーバプロセス（tadpole）と、その死活を監視するプロセス（treefrog）によって構成されています。サーバプロセスでは、1つのリクエストに対し1つのスレッドが割り当てられます。つまり、AP サーバはマルチプロセスかつマルチスレッドなアーキテクチャです。

万が一セグメンテーションフォルトなどで tadpole プロセスが落ちると、treefrog プロセスが検知して tadpole プロセスを再起動します。このようなフォールトトレラントな仕組みがあるので、サービス自体は継続して提供されます。

## マルチプロセッシングモジュール  -  MPM

アプリケーションサーバが並列処理（マルチプロセッシング）するための仕組み MPM（Multi-Processing Modules）として、次の２つが実装されています。どちらか１つ選択して設定ファイルに指定します。デフォルトは thread になっています。

* **thread:** リクエストがあるたびにスレッドが生成される。アクションはスレッド上で動作し、リクエストを処理してレスポンスを返すと、そのスレッドは消滅する。パフォーマンスが良い。
* **hybrid:** ディスクリプタはepoll により監視され、リクエストはスレッドで処理されるハイブリッド型の機構。Linux でのみ使用可能。C10K問題対策のために実装された。WebSocket 向け。


thread モジュールを採用した方が高い性能を出しますが、長くTCPセッションを接続しつづける WebSocket を使うアプリの場合は、hybrid モジュールを設定することも考えてください。

セグメンテーションフォルトなどのエラーが発生するとプロセスが落ちると、そこにあるスレッドも全て落ちてしまいます。あるスレッドで不具合があると、並行に処理している他のスレッドに影響する可能性があるということを気にしておいてください。
また、Web アプリケーションにメモリリークの不具合がありながら運用を続けると、いずれはメモリを食いつぶすことになるでしょう。TreeFrog の Web アプリではメモリアロケーションの必要性は低いですが、もしそうする場合はメモリリークには十分気をつけて実装しましょう。


## ベンチマーク

チュートリアルの章で作ったサンプルアプリ(blogapp)とベンチマークソフト httperf を使って、性能を計測してみました。

**テスト環境**

* PC: MacBook Pro  [ 2.4GHz Intel Core 2 Duo,  4GB 1067MHz DDR3 ]
* OS: Mac OS X 10.6
* DB: SQLite  （レコード：0件）
* Qt: 4.7  (Qt SDK 2010.05 版)
* TreeFrog: 0.54  (最適化オプション-O2でコンパイル)

**テスト方法**

* localhost の /Blog/index に向けて、１回の接続で１リクエストを大量に送信して性能を測る。httperf を使用。

フレームワークとしては、1 リクエストでコントローラ、モデル、DB、ビューをひとめぐりすることになります。キャッシュシステムは実装されていないので（0.54では）、DB に SQL クエリを毎回コールします。

thread モジュールの場合：

**パラメータ： MPM.thread.MaxServers=20**

```
 $ httperf –server=localhost –port=8800 –uri=/Blog/index/ –num-conns=10000 –num-calls=1
 httperf –client=0/1 –server=localhost –port=8800 –uri=/Blog/index/ –send-buffer=4096
 –recv-buffer=16384 –num-conns=10000 –num-calls=1
 httperf: warning: open file limit > FD_SETSIZE; limiting max. # of open files to FD_SETSIZE
 Maximum connect burst length: 1
 Total: connections 10000 requests 10000 replies 10000 test-duration 17.800 s

 Connection rate: 561.8 conn/s (1.8 ms/conn, <=1 concurrent connections)
 Connection time [ms]: min 0.3 avg 1.8 max 17.5 median 1.5 stddev 0.5
 Connection time [ms]: connect 0.1
 Connection length [replies/conn]: 1.000

 Request rate: 561.8 req/s (1.8 ms/req)
 Request size [B]: 73.0

 Reply rate [replies/s]: min 555.4 avg 562.3 max 566.8 stddev 6.1 (3  samples)
 Reply time [ms]: response 1.7 transfer 0.0
 Reply size [B]: header 313.0 content 421.0 footer 0.0 (total 734.0)
 Reply status: 1xx=0 2xx=10000 3xx=0 4xx=0 5xx=0

 CPU time [s]: user 3.13 system 13.73 (user 17.6% system 77.1% total 94.7%)
 Net I/O: 442.7 KB/s (3.6*10^6 bps)

 Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
 Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

※ 数回やって、中間くらいのものを貼りつけました↑

1秒間に約 562 回のリクエストを処理できています。これはレコードが0件の時の数値なので、TreeFrog Framework 自体が出せるほぼ最高の数値と言えるでしょう。Webアプリケーションフレームワークの中で、おそらくトップクラスの性能だと思っているのですが、どうでしょうか。

実際のWebアプリでは、コントローラやモデルのロジックがより複雑で、さらに DB に大量のレコードがあるでしょうから、これよりパフォーマンスは低下するでしょう。参考として考えて下さい。

##### 結論： MPM は 'thread' でいけ。WebSocket を使うなら 'hybrid' も検討せよ。
