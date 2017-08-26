---
title: 性能
page_id: "160.0"
---

## 性能

### 创建应用服务器

Treefrog应用服务器(AP server)创建了一个服务进程处理HTTP(*tadpole*)和一个进程监控它的生死(*treefrog*).

如果分段故障导致*tadpole*进程崩溃了, *treefrog*进程将会探测它然后重新启动*tadpole*进程. 这种故障容忍机制使得服务器能够持续地提供服务.

## 多处理模块(Multi-Processing Module)-MPM

这里有两种多处理模块(Multi-Processing Module)来创建应用服务器(AP server)的多处理模块: *prefork* 和 *thred*.它们类似于Apache. 你需要在配置文件中选择它们中的一个. 默认的是'thread'.

* **prefork:** 提前创建'fork'进程, 然后创建嵌套字(socket)作为"监听(listen)". 操作(action)在进程过程中执行, 当请求被处理和反馈返回时, 进程消失. 不要重复使用它! 如果操作(action)默认停止了或者非法操作, 它不会影响任何操作(action).  **[废弃]**
* **thread:** 当有请求时,每次购创建线程. 操作(action)在线程过程中执行, 当请求被处理完和响应发送后, 线程消失. 性能很好.
* **hybrid:** 嵌套字(sockets)被epoll系统调用监控, HTTP请求在线程中处理. 它只在Linux中可用. 它是为了解决能保持很多嵌套字(sockets)的C10K问题而实现的.

如果你使用thread模块, 性能将可以非常高(大约14倍于'prefork'). 如果在linux上使用hybrid模块, 性能在高负载时可以好很多. 然而, 当分段错误发生时, 每个处理会停止, 因此所有线程可以停止. 如果一些操作(action)有一个故障, 其他平行运行的操作(actions)也会受到影响. 那将导致问题. 使用prefork模块时, 处理被分成了独立的操作. 所以这个问题的担心可以避免.

此外, 使用thread或者hybrid模块, 当网页应用有内存泄漏错误时, 持续运行下内存很快或者不久就会被用光. 当然, 内存泄漏bug应该被修复, 但是如果你不能解决它, 你可以使用prefork模块.每次处理被退出可以避免内存被消耗光. 然而, 它是烦恼的.<br>

当选择一个MPM时请确保考虑到上面的事情.

## Benchmark


下面的对比使用了blogapp示例应用和benchmark软件*httperf*.

**测试环境:**

* PC: MacBook Pro [ 2.4GHz Intel Core 2 Duo, 4GB 1067MHz DDR3 ]
* OS: Mac OS X 10.6
* DB: SQLite (no record)
* Qt: 4.7 (Qt SDK 2010.05)
* TreeFrog: 0.54 (compiled by -O2 option)

**测试方法**

* 性能测试是通过在本机上在一个连接里发送大量的请求到*/blog/index*来测量的. 使用了Httperf.

这个框架, 独立的请求, 控制器, 模型, 数据库, 和视图都完全地被检查了. 因为DB缓存系统没有实现(0.54版), 每次都调用了一个SQL查询.

thread模块的例子:<br>
**参数: MPM.thread.MaxServers=20**

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

 Reply rate [replies/s]: min 555.4 avg 562.3 max 566.8 stddev 6.1 (3 samples)
 Reply time [ms]: response 1.7 transfer 0.0
 Reply size [B]: header 313.0 content 421.0 footer 0.0 (total 734.0)
 Reply status: 1xx=0 2xx=10000 3xx=0 4xx=0 5xx=0

 CPU time [s]: user 3.13 system 13.73 (user 17.6% system 77.1% total 94.7%)
 Net I/O: 442.7 KB/s (3.6*10^6 bps)

 Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
 Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

* 通过运行几次测试, 我能够找到中间结果, 如上面贴出来的.↑

大约每秒执行562次请求. 这个是记录为0时的数字, 因此它可以表现有能力的Treefrog框架的最高性能图. 我认为它展示了网页应用程序框架的top class性能. 你认为呢?

在真实的网页应用中, 控制器(controller)和模型(model)的逻辑是比较复杂的, 而且应该有大量的记录, 所以性能应该会必上面的低. 这些图应该作为参考.


##### 概要:使用'thread'作为多处理模块MPM. 如果使用WebSocket协议，请考虑设置"hybrid".
