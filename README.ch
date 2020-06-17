小巧强悍高效
================================

Treefrog框架是一个基于C++和Qt的高速全栈的网页应用框架, 支持HTTP和WebSocket协议.
用它开发的网页应用程序可以比用其它轻量级的编程语言运行得更快.
在程序开发过程中, 它提供了O/R映射系统和基于MVC体系的模版系统, 目标是通过惯例优于配置的原则实现快速开发.

特点
--------
 * 高性能  - C++引擎的高速优化的应用服务器
 * O/R 映射  - 隐藏了复杂的和麻烦的数据库访问
 * 模版系统  - 采用类似ERP的模板引擎
 * 支持多种数据库  - MySQL, PostgreSQL, ODBC, SQLite, Oracle, DB2,
                     InterBase, MongoDB和Redis
 * 支持WebSocket  - 提供双向通信通道
 * 生成器  - 自动生成程序骨架和Makefile文件
 * 支持各種響應類型  - JSON，XML和CBOR
 * 跨平台  - Windows, macOS, Linux等. 一次编写,到处编译
 * 开源  - New BSD License

安装需求
------------
TreeFrog使用了Qt的qmake构建系统.

网站
--------
 http://www.treefrogframework.org/

发行版本
--------
 https://github.com/treefrogframework/treefrog-framework/releases

文档
---------
 获得更多信息:
 http://treefrogframework.github.io/treefrog-framework/

API参考
-------------
 http://treefrogframework.org/tf_doxygen/classes.html

论坛
-----
 TreeFrog Framework的讨论组:
 https://groups.google.com/forum/#!forum/treefrogframework

MongoDB通信
---------------------
Treefrog框架使用10gen-supported的C驱动来和MongoDB服务器进行通信.
驱动的源代码也包含在这个包中. 也可查看它的README.