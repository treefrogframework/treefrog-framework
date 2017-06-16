---
title: Performance Comparison of Web Application Frameworks
page_id: "150.0"
---

## Performance Comparison of Web Application Frameworks

### Comparing the Throughput of Web Application Frameworks

Following the example of [cakepher's article](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}, I have tried to compare the web application framework throughput, not only using PHP but also MVC oriented. Due to my skills and my time, I was able to compare only four: CakePHP, Codeigniter, Ruby on Rails and TreeFrog Framework. I would have liked to include Java framework, but I was not able to because of a lack of knowledge.

* Measured the throughput when one record is obtained from the DB and then displayed.

### Note

In this article, "fast" is used to mean "throughput is high".

### Environment

```
 Server: Core2 Duo E4500 @ 2.20GHz / 2GB Memory / SATA HDD / 1Gbps Ethernet
 OS: Ubuntu 12.04 (32bit)
 PHP 5.3.10 with APC3.1.7
 Ruby 1.9.3p194
 Apache 2.2.22
 MySQL 5.5.24
```

### Schema of 'posts' Table

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

In the table, there are 10 records including Japanese text. Since I only wanted to measure the speed of the framework, I was able to just use 10 records because the DB should not be a bottleneck.

### Target Frameworks

* TreeFrog Framework 1.0
* CakePHP 2.2.0
* Codeigniter2.0.2
* Rails 3.2.6 (Apache+Passenger+Asset Pipeline ON)

### Measurement Tool

The Siege measurement looks at how many cases are processed in 3 seconds with 10 concurrent connections from a remote machine. It is considered that accurate measurement is not possible by using the tool on the same machine, so it is done remotely.

We conducted the test about 10 times per framework and took the average of the top 3 results. In other words, we compared against good numbers!

```
 $ siege -b -c 10 -t 3S http://192.168.x.x/controller/path
```

By the way, when measured with [code](https://github.com/ichikaway/CakePHP-PerformanceCheckSample/blob/master/php/view.php){:target="_blank"} written in Plain PHP, the result was 1261 trans/sec; a very high number.

If PHP code is very small, it can be very fast. On the other hand, it seems slower than expected when the amount of code and the amount of function calls is [large](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}. This may be because of its nature to be a scripting language.

### Result List

I have summarized the detailed results in a [spreadsheet](https://docs.google.com/spreadsheet/ccc?key=0AlpTorSDNQjbdEpWTURuRE5TaEtNN0FYbXU5Vl92RUE#gid=0){:target="_blank"}.

### Results Graph

Results are shown in the following graph. Higher values mean faster.

<div class="img-center" markdown="1">

![Result Graph]({{ site.baseurl }}/assets/images/documentation/snapshot4.png "Result Graph")

</div>

### Discussion

The results show that TreeFrog Framework is considerably fast. Although not as fast as Plain PHP, it is more than twice the speed of Condeingiter which has a reputation as being the fastest MVC framework in PHP. It means that it appears to implement the full strength C++. However, in reality, a reverse proxy such as nginx is sometimes used, in which case performance of the system as a whole, would be degraded. In the future, as we accumulate operational results, I will try to organize documentation on this issue.

Comparing the results the PHP framework (including Plain PHP) with cakephper, overall we can obtain slightly better results (by about 1.1 to 1.3 times) since the machine performance was a bit better than theirs. The ratio of the result is almost the same.

As I expected, Codeigniter is three times faster than Cake PHP. Codeigniter has plenty of functions so I was surprised to see this result. Cake PHP has many users and a good documentation. If speed is not the main factor, it can be then a good choice.

I had an image that Rails (Ruby) was slow, but it seems that this image is outdated. It cannot reach Codeigniter's speed, but it has almost doubled the throughput of CakePHP. However I was monitoring my operating system so I was aware that *Passenger* was consuming a lot of memory (I can't give figures unfortunately). Pursuit of speed seems to consume memory. As for Rails, there are faster web serves, but I'll discuss that later.

### Measured Applications

I used Cake PHP and Codeigniter from the [cakephper's articles](http://d.hatena.ne.jp/cakephper/20110802/1312275110){:target="_blank"}.

Rails: [Here >>](https://docs.google.com/open?id=0B1pTorSDNQjbT2t3Ylc1Wl9aUzg){:target="_blank"}

TreeFrog Framework: [Here >>](https://docs.google.com/open?id=0B1pTorSDNQjbNldxT1NjbEs4VzQ){:target="_blank"}

**make commands:**

```
 $ qmake "CONFIG+=release" -recursive -spec linux-g++
 $ make
```

### P.S.

Unfortunately, I cannot give you specific figures since I just measured it by eye-sight, but when the load is on, the amount of memory usage (%) was the lowest when using TreeFrog Framework.
TreeFrog Framework is a multithreaded model, whereas Codeigniter and CakePHP use the multiprocessing model (Apache prefork). I think this difference can significantly affect the result. However, when combining TreeFrog Framework and reverse proxy (such as nginx), the amount of memory usage would be about the same. I would like to check this in the future.

With all that in mind, I would like to show the result of top command under load, for both TreeFrog Framework and Codeigniter.

**Top command screen when TreeFrog Framework has high load:**

<div class="img-center" markdown="1">

![Top Command Screen 1]({{ site.baseurl }}/assets/images/documentation/snapshot2-2.png "Top Command Screen 1")

</div>

**Top command screen when Codeigniter has high load:**

<div class="img-center" markdown="1">

![Top Command Screen 2]({{ site.baseurl }}/assets/images/documentation/snapshot3-2.png "Top Command Screen 2")

</div>