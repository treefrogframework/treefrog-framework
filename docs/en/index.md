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
  4. Supported databases  - MySQL, PostgreSQL, ODBC, SQLite, MongoDB, Redis, etc.
  5. WebSocket support  - Providing full-duplex communications channels
  6. Generator  - Automatically generates scaffolds and Makefiles
  7. Various response types  - JSON, XML and CBOR
  8. Cross-platform  - Same source code can work on Windows, macOS and Linux
  9. OSS  - New BSD License


## <i class="fa fa-comment" aria-hidden="true"></i> Selecting TreeFrog Framework

It is said that there is a trade-off relationship between development efficiency and operation speed in web application development, is it really true?

There is not such a thing. it is possible to develop efficiently by providing convenient development tools and excellent libraries from the framework and by specifying specifications to minimize the configuration file.

In recent years, cloud computing has emerged, the importance of Web applications is increasing year by year. Although it is known that the execution speed of the script language decreases as the amount of code increases, C++ language can operate at the fastest speed with a small memory footprint and does not decrease execution speed even as the amount of code increases.

Multiple application servers running in scripting language can be aggregated into one without degrading performance.
Try TreeFrog Framework which combines high productivity and high speed operation!


## <i class="fa fa-bell" aria-hidden="true"></i> News

Jun. 19, 2021

### TreeFrog Framework version 2.0 (beta2) release <span style="color: red;">New!</span>

 - Updated the scaffold generator to generate WebAPI codes.
 - Modified the scaffold generator to generate service classes for the model layer.

  [<i class="fas fa-download"></i> Download this version](/en/download/)

May 23, 2021

### TreeFrog Framework version 2.0 (beta) release <span style="color: red;">New!</span>

 - Support for Qt version 5 and version 6.
 - Modified not to use obsolete functions of Qt.

Feb. 6, 2021

### TreeFrog Framework version 1.31.0 (stable) release

 - Fix a bug of TMultiplexingServer (epoll server).
 - Modified not to use obsolete functions of Qt.
 - Added TAbstractSqlORMapper class.
 - Performance improvement.

Aug. 21, 2020

### TreeFrog Framework version 1.30.0 (stable) release

  - Implemented logics for X-Forwarded-For Header.
  - Implemented ActionMailer.smtp.RequireTLS parameter in application.ini.
  - Added a option for showing URL routing to treefrog command.
  - Updated I/F of ORM functions.
  - Performance improvement.

May. 5, 2020

### TreeFrog Framework version 1.29.0 (stable) release

  - Fix a bug of max-age of cookie.
  - Fix a bug of generating select-tag.
  - Modified to initialize boolean fields in classes generated.
  - Implemented publish() function in TActionController class.

Feb. 11, 2020

### TreeFrog Framework version 1.28.0 (stable) release

  - Implemented to add a SameSite attribute to cookie.
  - Modified to add a max-age value to cookie.
  - Fix a bug of listing available controllers.
  - Fix a bug of showing a port number by -l option.
  - Fix a bug of content type in renderText() function.

Dec. 5, 2019

### TreeFrog Framework version 1.27.0 (stable) release

  - Implemented OAuth2 client.  [Experimental]
  - Supports for MongoDB version 3.2 and later.
  - Fix a bug that timer for TSqlDatabasePool stops. #279
  - Fix a bug not to set all databases to transaction state value.
  - Fix a bug of execution of diff in tspawn command.

Oct. 19, 2019

### TreeFrog Framework version 1.26.0 (stable) release

  - Added cache modules by SQLite, MongoDB and Redis.
  - Updated LZ4 compression algorithm to 1.9.2.
  - Fix a compilation error on Ubuntu 19.10.
  - Changed the epoll MPM from multi-thread to single thread architecture.
  - Performance improvement.

July 20, 2019

### TreeFrog Framework version 1.25.0 (stable) release

  - Updated Mongo C driver to v1.9.5.
  - Updated LZ4 compression algorithm to v1.9.1.
  - Modified to set a domain in session cookie.
  - Changed the default C++ config from c++11 to c++14.
  - Other bugfixes and improvements.

Apr. 29, 2019

### TreeFrog Framework version 1.24.0 (stable) release

  - Modified to use LZ4 compression algorithm.
  - Implemented functions storing data of hash type for Redis.
  - Fix a bug of incorrect Content-Type returned for files with uppercase extension.
  - Fix a bug of parsing form-data values included backslashes.
  - Other bugfixes and improvements.

Jan. 6, 2019

### TreeFrog Framework version 1.23.0 (stable) release

  - Added failsafe of SQL when BEGEN/COMMIT/ROLLBACK failed.
  - Fix a bug of sqlobject for session store.
  - Minor changes of exception classes.
  - Added log messages for query log.
  - Other bugfixes.

Jun. 10, 2018

### TreeFrog Framework version 1.22.0 (stable) release

  - Support for CMake build of Web application.
  - Support SMTP connections to older servers not supporting ESMTP.
  - Fix bugs of tspawn.pro file.


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

[Benchmarks <i class="fas fa-external-link-alt"></i>](https://www.techempower.com/benchmarks/#section=data-r16){:target="_blank"}
