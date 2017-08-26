---
title: 网页应用框架的性能对比
page_id: "150.0"
---

## 网页应用框架的性能对比

### 网页应用框架的产出效率对比

按照[cakepher’s article](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}的例子, 我尝试着对比了网页应用框架的产出率, 不仅仅使用PHP, 还面向MVC.基于我的技能和时间, 我只对比了4个: CakePHP, Codeigniter, Ruby on Rails和Treefrog框架, 我还想包括Java框架, 但是由于我缺少这方面的知识, 没能实现它.

* 测量当一条记录从数据库获得然后显示的产出率.

### 说明

在这篇文章里, "快(fast)"用来表示产出率很高.

### 环境

```
Server: Core2 Duo E4500 @ 2.20GHz / 2GB Memory / SATA HDD / 1Gbps Ethernet
OS: Ubuntu 12.04 (32bit)
PHP 5.3.10 with APC3.1.7
Ruby 1.9.3p194
Apache 2.2.22
MySQL 5.5.24
```

### 'posts'表的结构

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

在表中, 已经有了10条包含日本文字的记录. 因为我只想测量框架的速度, 我只使用了10条记录, 因为数据库不会成为一个瓶颈.

### 目标框架

* TreeFrog Framework 1.0
* CakePHP 2.2.0
* Codeigniter2.0.2
* Rails 3.2.6 (Apache+Passenger+Asset Pipeline ON)

### 测量工具

Siege测量看在3秒内10个并发连接从远程机器发起请求有多少被处理. 考虑到使用这个工具在同一台机器上进行精确的测量是不可能的, 所以测试是从远程上发起的.
我们每个框架进行了大约10次测试, 然后取最大的3个记录的平均值. 换句话说, 我们比较最好的结果.

```
$ siege -b -c 10 -t 3S http://192.168.x.x/controller/path
```

通过这样的方式, 测量PHP写的[代码](https://github.com/ichikaway/CakePHP-PerformanceCheckSample/blob/master/php/view.php){:target="_blank"}, 结果是1261 trans/sec, 一个非常高的数字.

如果PHP代码是非常小的, 它可以非常快. 在另一方面, 当代码量和函数调用的量[大](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}时, 比预期的低很多. 这是个可能是因为脚本语言的性质.

### 结果列表

我已经在[电子表格](https://docs.google.com/spreadsheet/ccc?key=0AlpTorSDNQjbdEpWTURuRE5TaEtNN0FYbXU5Vl92RUE#gid=0){:target="_blank"}中将详细的结果汇总了.

### 结果图表

结果显示在下面的图表中. 柱子高表示快.

<div class="img-center" markdown="1">

![图表结果]({{ site.baseurl }}/assets/images/documentation/snapshot4.png "Result Graph")

</div>

### 讨论

结果显示了Treefrog框架是相当的快. 虽然没有纯PHP这么快, 它已经是PHP里最快的MVC框架Codeingiter速度的两倍了. 它意味着它似乎实现了C++的高效运行.然而, 事实上, 有些时候会使用反向代理(如nginx), 整个系统的性能会降低.在将来, 根据累计的运行的结果, 我将尝试组织关于这个问题的文档.

对比其他cakephp使用者和PHP框架(包括纯PHP)的结果, 总体上我们可以获得一个比较好的结果(大约1.1 到1.3倍), 因为机器性能比它们的好一点点. 结果的倍率几乎都是一样的.

如我期待的一样, Codeigniter比Cake PHP快三倍. Codeigniter有大量的函数, 所以我看到这个结果很惊讶. Cake PHP有很多使用者和很好的文档. 如果速度不是主要的因素, 它可以是一个好的选择.

我曾经有一个低速度的Rails(Ruby)的图片, 但是它看起来图片已经过期了. 它不能达到Codeigniter的速度, 但是它的生产力差不多是CakePHP的两倍. 然而观察我的操作系统注意到*Passenger*消耗了大量的内存(不幸,我不能给出图片). 追求速度看起来消耗内存. 有比较快的Rails网页服务器, 但是我将以后讨论.

### 测量应用

我使用了[cakephper’s articles](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}的Cake PHP和Codeigniter.

Rails: [这里 >>](https://docs.google.com/open?id=0B1pTorSDNQjbT2t3Ylc1Wl9aUzg){:target="_blank"}

TreeFrog框架: [这里 >>](https://docs.google.com/open?id=0B1pTorSDNQjbNldxT1NjbEs4VzQ){:target="_blank"}

**make 命令:**

```
$ qmake "CONFIG+=release" -recursive -spec linux-g++
$ make
```

### 附言

不幸, 我不能给你这些图片因为我只是通过目视来测量的, 但是当负载上来后, 使用Treefrog框架的内存使用量(%)是最低的.
Treefrog框架是多线程的, 而Codeigniter和CakePHP使用多进程(Apache prefork). 我认为这个不同可以明显的影响结果. 然而, 当组合使用Treefrog框架和反向代理(如nginx), 内存的使用量将会变得差不多. 我将会在将来检查这个问题.

综上所述, 我将显示负载最多的程序结果, Treefrog框架和Codeigniter两个都有.

**Treefrog框架高负载时负载最多的程序:**

<div class="img-center" markdown="1">

![Top Command Screen 1]({{ site.baseurl }}/assets/images/documentation/snapshot2-2.png "Top Command Screen 1")

</div>

**Codeigniter框架高负载时负载最多的程序:**

<div class="img-center" markdown="1">

![Top Command Screen 2]({{ site.baseurl }}/assets/images/documentation/snapshot3-2.png "Top Command Screen 2")

</div>