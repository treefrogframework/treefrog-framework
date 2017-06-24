---
title: Home
page_id: "home.00"
---

## Small but Powerful and Efficient

TreeFrog Framework is a high-speed and full-stack C++ framework for developing Web applications, which supports HTTP and WebSocket protocol.

Because the sever-side framework was written in C++/Qt, web applications can run faster than that of scripting language. In application development, it provides an O/R mapping system and template systems on an MVC architecture, aims to achieve high productivity through the policy of  convention over configuration.

## Latest News

May 27, 2017

### TreeFrog Framework version 1.17.0 (stable) release <span style="color: red;">New!</span>

  - Fix a bug of comparisn logic of If-Modified-Since header.
  - Fix a bug of URL path traversal.
  - Added logic of routing to a static file.
  - Added a class to process in background, TBackgroundProcess.
  - Other bugfixes.

 <a href="{{site.baseurl}}/download/en"><span style="color: blue;">Download this version</span></a>

Apr 8, 2017

### TreeFrog Framework version 1.16.0 (stable) release

  - Added a config for listening IP address.
  - Added a config for executing SQL statements on post-open.
  - Added a function for multi-fields 'order by', tfGetModelListByCriteria().
  - Added pages for GigHub Pages, English and Japanese.
  - Other bugfixes.

Jan 22, 2017

### TreeFrog Framework version 1.15.0 (stable) release

  - Added debug functions like 'tDebug() << "foo" '.
  - Added config-initializer functions to TWebApplication class.
  - Added C++11 for-loop for TSqlORMapper class.
  - Modified functions of TFormValidator class.
  - Other bugfixes.

Dec 5, 2016

### TreeFrog Framework version 1.14.0 (stable) release

  - Modified to use QThreadStorage class instead of thread_local.
  - Modified the scaffold generater to generate better codes.
  - Added '#partial' tag in ERB.
  - Fix a bug of renderPartial() funcion on Windows.
  - Fix a bug of session sqlobject store in PostgreSQL.
  - Performance improvement.
  - Other bugfixes.


##### WANTED
 - Developers, testers.

Please email me or ML. Welcome!

## Features

  1. High performance - Highly optimized Application server engine of C++.  Benckmarks by 3rd party.
  2. O/R mapping  - Conceals complex and troublesome database accesses.
  3. Template system  - Completely divided templates and presentation logic.
  4. Support for many DB – MySQL, PostgreSQL, ODBC, SQLite, Oracle, DB2, InterBase, MongoDB and Redis.
  5. Cross-platform  - Windows, Mac OS X, Linux, etc.  Same source code can work on other platforms.
  6. Support WebSocket – Providing full-duplex communications channels.
  7. Generator – Automatically generates scaffolds and Makefiles.
  8. Less resource -  Stable operation even on Raspberry Pi.
  9. OSS  - New BSD License
