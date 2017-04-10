Small but Powerful and Efficient
================================

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
 * Template system  - Completely divided templates and presentation logic
 * Support for many DB  - MySQL, PostgreSQL, ODBC, SQLite, Oracle, DB2,
                          InterBase, MongoDB and Redis.
 * Support WebSocket  - Providing full-duplex communications channels
 * Generator  - Generates scaffolds and Makefiles automatically
 * Cross-platform  - Windows, macOS, Linux, etc. Write once, compile
                     anywhere.
 * Ajax support  - JSON, XML and Plain text available
 * Less resource  -  Stable operation even on Raspberry Pi
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
 Get additional information on the site:
 http://www.treefrogframework.org/documents

API Reference
-------------
 http://treefrogframework.org/tf_doxygen/classes.html

Forum
-----
 Discussion group for TreeFrog Framework:
 https://groups.google.com/forum/#!forum/treefrogframework

MongoDB communication
---------------------
TreeFrog Framework uses the 10gen-supported C driver to communicate with the
MongoDB server. The source code of the driver is included in this package.
See the README also.



Composite primary keys support
================================
This TreeFrog Framework support composite primary keys.

Changed files
-----------------
src:

	tsqlobject.h
	tsqlobject.cpp

	tsessionobject.h

	tcriteria.h
	tcriteria.cpp
	
	tsqlormapper.h

tools:

	tableschema.h
	tableschema.cpp
	
	abstractobjgenerator.h
	sqlobjgenerator.h
	sqlobjgenerator.cpp
	mongoobjgenerator.h
	mongoobjgenerator.cpp
	
	modelgenerator.h
	modelgenerator.cpp
	
	controllergenerator.h
	controllergenerator.cpp
	
	otamagenerator.h
	otamagenerator.cpp
	
	erbgenerator.h
	erbgenerator.cpp
