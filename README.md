Small but Powerful and Efficient
================================

[![ActionsCI](https://github.com/treefrogframework/treefrog-framework/actions/workflows/actions.yml/badge.svg)](https://github.com/treefrogframework/treefrog-framework/actions/workflows/actions.yml)
[![CircleCI](https://circleci.com/gh/treefrogframework/treefrog-framework.svg?style=shield)](https://circleci.com/gh/treefrogframework/treefrog-framework)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Release](https://img.shields.io/github/v/release/treefrogframework/treefrog-framework.svg)](https://github.com/treefrogframework/treefrog-framework/releases)
[![Docker image](https://img.shields.io/badge/Docker-image-blue.svg)](https://hub.docker.com/r/treefrogframework/treefrog/)

TreeFrog Framework is a high-speed and full-stack web application framework
based on C++ and Qt, which supports HTTP and WebSocket protocol. Web
applications can run faster than that of lightweight programming language.
In application development, it provides an O/R mapping system and template
system on an MVC architecture, aims to achieve high productivity through the
policy of convention over configuration.

Features
--------
 * High performance  - Highly optimized Application server engine of C++
 * O/R mapping  - Conceals complex and troublesome database accesses
 * Template system  - ERB-like template engine adopted
 * Supports for many DB  - MySQL, PostgreSQL, ODBC, SQLite, Oracle, DB2,
   InterBase, MongoDB and Redis.
 * WebSocket support  - Providing full-duplex communications channels
 * Generator  - Generates scaffolds and Makefiles automatically
 * Supports various response types  - JSON, XML and CBOR
 * Cross-platform  - Windows, macOS, Linux, etc. Write once, compile anywhere.
 * OSS  - New BSD License

Requirements
------------
TreeFrog uses the qt qmake build system.

Web Site
--------
 http://www.treefrogframework.org/

Releases
--------
 https://github.com/treefrogframework/treefrog-framework/releases

Documents
---------
 Get additional information:
 http://treefrogframework.github.io/treefrog-framework/

API Reference
-------------
 http://api-reference.treefrogframework.org/annotated.html

Forum
-----
 Discussion group for TreeFrog Framework:
 https://groups.google.com/forum/#!forum/treefrogframework

Docker Images
-------------
  https://hub.docker.com/r/treefrogframework/treefrog/

```
  $ docker pull treefrogframework/treefrog
```
  Docker files are stored in the 'dockerfiles' directory of the 'docker' branch.

MongoDB communication
---------------------
TreeFrog Framework uses the 10gen-supported C driver to communicate with the
MongoDB server. The source code of the driver is included in this package.
See the README also.
