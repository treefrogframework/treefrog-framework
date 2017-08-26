---
title: 色々なWebアプリケーションフレームワークのパフォーマンスを比較
page_id: "150.0"
---

## 色々なWebアプリケーションフレームワークのパフォーマンスを比較

### スループットを比較してみた

[cakephper さんの記事](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}をまねて、PHPのみならずMVC指向のWebアプリケーションフレームワークのスループットを比較してみました。私のスキルと時間の関係で、CakePHP, Codeigniter, Ruby on Rails そしてTreeFrog Frameworkの４つだけになってしまいました。Javaフレームワークを含めたかったですが、残念ながら知識不足でできませんでした（他力本願）。

* 単純にDBから１件のレコードを取得し表示するまでのスループットを計測します。

### 注意点

cakephper さんの記事にもありますが、ここでの**"速い"**とは**スループットが高い**ことを指しています。

### 環境

```
 Server: Core2 Duo E4500 @ 2.20GHz / 2GB Memory / SATA HDD / 1Gbps Ethernet
 OS: Ubuntu 12.04 (32bit)
 PHP 5.3.10 with APC3.1.7
 Ruby 1.9.3p194
 Apache 2.2.22
 MySQL 5.5.24
```

※ できれば、MySQLを別のマシンに置きたかったですが、それがなかったもので。

### postsテーブルのスキーマ

```sql
 CREATE TABLE IF NOT EXISTS `posts` (
   `id` int(11) NOT NULL AUTO_INCREMENT,
   `name` varchar(255) NOT NULL,
   `text` text,
   `created` datetime DEFAULT NULL,
   `modified` datetime DEFAULT NULL,
   PRIMARY KEY (`id`)
 ) ENGINE=InnoDB  DEFAULT CHARSET=utf8;
```

テーブルには、日本語のテキストを含めた１０件のレコードがある状態にします。純粋にフレームワークの速度を計測したかったので、DBがボトルネックにならぬよう１０件に抑えました。

### 対象フレームワーク

* TreeFrog Framework 1.0
* CakePHP 2.2.0
* Codeigniter2.0.2
* Rails 3.2.6 (Apache+Passenger+Asset Pipelineオン)

### 計測ツール

Seigeという計測ツールを使い、リモートマシンから同時接続数10で3秒間に何件さばけるかを計測します。同一マシンでツールをつかうと正しい計測ができないと考えられるので、リモートから行います。

１フレームワークにつき１０回ほど実施し、上位３件の平均を求めました。いいとこどり。

```
 siege -b -c 10 -t 3S http://192.168.x.x/controller/path
```

ちなみにPlain PHPで書いた[コード](https://github.com/ichikaway/CakePHP-PerformanceCheckSample/blob/master/php/view.php){:target="_blank"}で計測すると、1261 trans/secという結果がでました。相当高い数字ですね。

PHPは単純なコードだと、相当速いことがよく分かりました。反面、コード量や関数コールが多いと想像以上に[遅くなる](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}ようです。スクリプト言語の宿命なのでしょうか。

### 結果（一覧表）

細かい結果はこちらの[スプレッドシート](https://docs.google.com/spreadsheet/ccc?key=0AlpTorSDNQjbdEpWTURuRE5TaEtNN0FYbXU5Vl92RUE#gid=0){:target="_blank"}にまとめています。

### 結果(グラフ)

結果は下記の通り。値が高いものほど速いです。

<div class="img-center" markdown="1">

![結果]({{ site.baseurl }}/assets/images/documentation/snapshot4.png "結果")

</div>

### 考察

TreeFrog Framework が相当速いという結果になりました。Plain PHP には及びませんが、PHPのMVCフレームワークの中で最も速いという評判のCodeigniterの2倍強の数字を叩き出しました。フルC++で実装されている強みがでたと言えます。ただし、実運用ではnginxなどのリバースプロキシを置くこともあるので、その場合システム全体として若干パフォーマンスが低下するでしょう。後発のフレームワークということで、今後はドキュメントの整備と動作実績を積み上げる必要があるでしょう。（頑張ります）

PHP フレームワーク（PlainPHP含む）の結果について、cakephper さんのものと比べると、マシン環境の性能が若干高い分だけ全体的に少し良い数字がでています（1.1〜1.3倍）。結果の比率はほぼ同様になりました。やはりCodeigniterがCakePHPの約３倍速かったです。Codeigniter には十分な機能が備わっているにもかかわらず、この数字には驚きです。CakePHPには、ユーザが多くドキュメントも豊富なので、速度をそれほど追求しないシステムでは良い選択になりえます。

Rails(Ruby)に対しては遅いイメージを持っていた私でしたが、今となっては過去の話と言えるでしょう。Codeigniter には及ばないものの、CakePHP の倍近いスループットがでています。ただし、システムモニターをたまたま起動していたことで気づきましたが、Passenger は非常にメモリを食うことが分かりました（残念ながら数字は出せていません）。速さを追求した分、メモリを消費するようです。Railsにはさらに速いWebサーバがでてきているようですが、また今度ということで。

### 計測したアプリケーション

CakePHP と Codeigniter は、cakephper さんの[記事に載せてあるもの](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}を利用させて頂きました。

Rails： [こちらへ >>](https://docs.google.com/open?id=0B1pTorSDNQjbT2t3Ylc1Wl9aUzg){:target="_blank"}

TreeFrog Framework: [こちらへ >>](https://docs.google.com/open?id=0B1pTorSDNQjbNldxT1NjbEs4VzQ){:target="_blank"}

**makeコマンド：**

```
 $ qmake -r  "CONFIG+=release"
 $ make
```

### 追記

メモリ使用量について

topコマンドを使った目視なので説得力にかけるのですが、負荷がかかっている時のメモリ使用量（%）はTreeFrog Frameworkが最も少なかったです。TreeFrog FrameworkはC++のマルチスレッドモデル、Codeigniter とCakePHP はスクリプト言語のマルチプロセスモデル（Apache prefork）であることが大いに影響していると言えるでしょう。ただし、TreeFrog Framework とリバースプロキシ（nginxなど）を組み合わせる構成にした場合、それほど変わらない使用量になるかもしれません。機会があったらきちんと調べたいところです。

ということで、負荷をかけている時のTreeFrog FrameworkとCodeigniterについて、それぞれtopコマンドの結果を以下に貼り付けます。

**TreeFrog Framework 高負荷時の top コマンド画面**

<div class="img-center" markdown="1">

![top コマンド画面 1]({{ site.baseurl }}/assets/images/documentation/snapshot2-2.png "top コマンド画面 1")

</div>

**Codeigniter 高負荷時の top コマンド画面**

<div class="img-center" markdown="1">

![top コマンド画面 2]({{ site.baseurl }}/assets/images/documentation/snapshot3-2.png "top コマンド画面 2")

</div>
