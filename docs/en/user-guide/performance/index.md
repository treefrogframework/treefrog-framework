---
title: Performance
page_id: "160.0"
---

## Performance

### Creating the Application Server

The TreeFrog Application server (AP server) is created by a server process that handles HTTP (*tadpole*) and a process that monitors its life and death (*treefrog*).

If a segmentation fault causes the *tadpole* process to go down, the *treefrog* process will detect it and then restart the *tadpole* process. This kind of fault-tolerant mechanism makes it possible for the service itself to be provided continuously.

## Multi-Processing Module – MPM

There are two MPMs (Multi-Processing Modules) to create multiprocessing modules for the application server (AP server): *prefork* and *thread*. These are similar to Apache. You need to choose one of them and then specify it in setting file. The default is 'thread'.

* **prefork:** Create the process "fork" in advance, and then create socket as "listen". The action is performed during the process, and when the request has been operated and the response returned, the process disappears. Do not reuse it! If the action is down by fault or illegal operation, it wouldn't then affect any actions.  **[Deprecated]**
* **thread:** Thread is created every time when there is a request. Action runs on the thread and when the request has been processed and the response sent, the thread disappears. The performance is good.
* **hybrid:** Sockets are monitored by epoll system call and an HTTP request is processed on a thread. It's available on Linux only. It is implemented for the C10K problem that can maintain many sockets.

If you use the thread module, performance can be very high (about 14x of 'prefork'). If you use the hybrid module on Linux, performance can be much better at high level load. However, when, for example, a segmentation fault occurs, each process can go down, therefore all threads can be down. If a certain action has a fault, other parallel running actions could also be affected. That would cause problems. In the case of the prefork module the process is divided into individual actions, so that this kind of concern can be avoided.

In addition, in case of using the thread or hybrid module setting, when a web application has a memory leak fault, by continuing operation, sooner or later the memory will be used up. Of course, the memory leak bug should be fixed, but if you cannot solve it, you can use the prefork module. Each time a process is exited you can avoid the memory being eaten up. However, it is annoying!<br>
Be sure to consider the mentioned things above when choosing an MPM.

## Benchmark

The following comparisons use a sample application (blogapp) and the benchmark software *httperf*.

**Test environment:**

* PC: MacBook Pro [ 2.4GHz Intel Core 2 Duo, 4GB 1067MHz DDR3 ]
* OS: Mac OS X 10.6
* DB: SQLite (no record)
* Qt: 4.7 (Qt SDK 2010.05)
* TreeFrog: 0.54 (compiled by -O2 option)

**Test Method**

* The performance here is measured by sending huge amount of requests to */blog/index* in localhost in one connection. Httperf is used.

The framework, individual requests, controller, model, DB, and view are all comprehensively checked. Since the DB cache system is not implemented (in the case of 0.54), an SQL query is called to the DB each time.

In the case of the thread module:<br>
**Parameter: MPM.thread.MaxServers=20**

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

* By running the test several times, I was able to find an intermediate result, as posted above ↑

About 562 times requests are executed per second. This is the number when the record is 0, so that it would represents the highest performance figure of which TreeFrog is capable. I think it indicates top class performance in the web application framework. What do you think?

In real web applications, the logic of controller and model is more complicated, and there should would be a large number of records, so performance would be less than the above. These figures should therefore be taken as a reference.

##### In brief: Use 'thread' as your MPM. If using the WebSocket protocol, consider setting 'hybrid' as well.
