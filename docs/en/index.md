---
title: Home
page_id: "home.00"
---

## <i class="fa fa-bolt" aria-hidden="true"></i> Small but Powerful and Efficient

TreeFrog Framework is a high-speed and full-stack C++ framework for developing Web applications, which supports HTTP and WebSocket protocol.

Web applications can run faster than that of scripting language because the server-side framework was written in C++/Qt. In application development, it provides an O/R mapping system and template systems on an MVC architecture, aims to achieve high productivity through the policy of  convention over configuration.


## <i class="fa fa-flag" aria-hidden="true"></i> Features

  1. High performance - Highly optimized Application server engine of C++.
  2. O/R mapping  - Conceals complex and troublesome database accesses
  3. Template system  - ERB-like template engine adopted
  4. Supported databases  - MySQL, MariaDB, PostgreSQL, ODBC, MongoDB, Redis, Memcached, etc.
  5. WebSocket support  - Providing full-duplex communications channels
  6. Generator  - Generates scaffolds, Makefiles and vue.js templates
  7. Various response types  - JSON, XML and CBOR
  8. Multi-platform  - Same source code can work on Windows, macOS and Linux
  9. OSS  - New BSD License


## <i class="fa fa-comment" aria-hidden="true"></i> Selecting TreeFrog Framework

It is said that there is a trade-off relationship between development efficiency and operation speed in web application development, is it really true?

There is not such a thing. it is possible to develop efficiently by providing convenient development tools and excellent libraries from the framework and by specifying specifications to minimize the configuration file.

In recent years, cloud computing has emerged, the importance of Web applications is increasing year by year. Although it is known that the execution speed of the script language decreases as the amount of code increases, C++ language can operate at the fastest speed with a small memory footprint and does not decrease execution speed even as the amount of code increases.

Multiple application servers running in scripting language can be aggregated into one without degrading performance.
Try TreeFrog Framework which combines high productivity and high speed operation!


## <i class="fa fa-bell" aria-hidden="true"></i> News

Sep. 27, 2025
### TreeFrog Framework version 2.11.2 (stable) release <span style="color: red;">New!</span>

 - Updated the internally used MongoDB C Driver to version 2.1.0.
 - Added support for std::format style formatting in logging functions.

  [<i class="fas fa-download"></i> Download this version](/en/download/)

Jul. 5, 2025
### TreeFrog Framework version 2.11.1 (stable) release

 - Fixed a compilation error bug in tftest.h.
 - Updated test macros.

May 5, 2025
### TreeFrog Framework version 2.11.0 (stable) release

 - Vite + Vue scaffolding support (experimental), see [Vite+Vue](/en/user-guide/view/vite+vue.html).
 - Fix a bug of TSharedMemory on macos.

Nov. 30, 2024
### TreeFrog Framework version 2.10.0 (stable) release

 - Supported for std::format style log output, Tf::error(), Tf::warn(), Tf::info(), etc.
 - Fix a bug of tsharedmemory initiation.
 - Only Qt6 supported after this version (Qt5 not supported).

Jun. 16, 2024
### TreeFrog Framework version 2.9.0 (stable) release

 - Fix a bug in the access log output regarding status codes.
 - Modified to set NULL into DB when the value is QString() in TSqlObject.
 - Added a function, TAbstractModel::setProperties(const QJsonObject &properties).
 - Updated mongoc driver to v1.26.2.
 - Updated glog to v0.7.0.

Dec. 10, 2023
### TreeFrog Framework version 2.8.0 (stable) release

 - Support prepared statements for PostgreSQL and MySQL.
 - Fix compilation error on Emscripten.

Mar. 26, 2023
### TreeFrog Framework version 2.7.1 (stable) release

  - Fix a bug of opening shared memory KVS.
  - Modified to reply NotFound when it can not invoke the action.

Feb. 25, 2023
### TreeFrog Framework version 2.7.0 (stable) release

  - Fix possibility of thread conflicting when receiving packets.
  - Changed hash algorithm to HMAC of SHA3.
  - Added Memcached as session store.
  - Updated malloc algorithm of TSharedMemoryAllocator.
  - Updated system logger.
  - Performance improvement for pooling database connections.

 [<i class="fa fa-list" aria-hidden="true"></i> All changelogs](https://github.com/treefrogframework/treefrog-framework/blob/master/CHANGELOG.md)

## <i class="fas fa-hand-holding-usd"></i> Support Development

TreeFrog Framework is New BSD licensed open source project and completely free to use. However, the amount of effort needed to maintain and develop new features for the project is not sustainable without proper financial backing. We accept donations from sponsors and individual donors via the following methods:

 - Donation with [PayPal <i class="fas fa-external-link-alt"></i>](https://www.paypal.me/aoyamakazuharu)
 - Become a [sponsor in GitHub](https://github.com/sponsors/treefrogframework)
 - BTC Donation. Address: [12C69oSYQLJmA4yB5QynEAoppJpUSDdcZZ]({{ site.baseurl }}/assets/images/btc_address.png "BTC Address")

I would be pleased if you could consider donating. Thank you!


## <i class="fa fa-user" aria-hidden="true"></i> Wanted

 - Developers, testers, translators.

Since this site is built with [GitHub Pages](https://pages.github.com/), translations can also be sent by a pull-request.
Visit [GitHub](https://github.com/treefrogframework/treefrog-framework){:target="_blank"}. Welcome!


## <i class="fa fa-info-circle" aria-hidden="true"></i> Information

[TreeFrog forum <i class="fas fa-external-link-alt"></i>](https://groups.google.com/forum/#!forum/treefrogframework){:target="_blank"}

Twitter [@TreeFrog_ja <i class="fas fa-external-link-alt"></i>](https://twitter.com/TreeFrog_ja){:target="_blank"}

[Docker Images <i class="fas fa-external-link-alt"></i>](https://hub.docker.com/r/treefrogframework/treefrog/){:target="_blank"}

[Benchmarks <i class="fas fa-external-link-alt"></i>](https://www.techempower.com/benchmarks/){:target="_blank"}
