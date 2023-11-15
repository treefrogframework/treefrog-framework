---
title: Introduction
page_id: "010.0"
---

## Introduction

### What is the TreeFrog Framework

TreeFrog Framework is a full-stack Web application framework. Written in C++, it is lightweight (low resource demands), and allows extremely fast working.

With the aim of reducing development costs while producing a C++ framework, a policy of "convention over configuration" has been followed. The configuration file has been made as small as possible. Because it provides help in automatic generation of code for template systems (scaffolding), O/R mapping and ORM, developers are free to focus on logic.

### Cross-platform

TreeFrog Framework is cross-platform. It runs on Windows, of course, but also on UNIX-like Operating Systems, macOS, and Linux. Using Windows open-source coding, it is possible to support Linux. Web applications that run on multiple platforms are also possible, simply by recompiling the source code.

### Controller

Developers will be easily able to obtain the data representing HTTP request/response, and the session. In addition to this, it has already provided same useful rough features such as login authentication, form validation, and access control.
Also, because it provides a mechanism (routing system) for call methods by the appropriate controller from the requested URL to required actions, there is no need to write rules one-by-one in the configuration file, or to distribute any action requests.

### View

In the view layer, ERB format descriptions, well known in Rails, may be used. The C++ code <% …%> can be embedded in HTML files. This means that coding can be written in a way very similar to a scripting language.
It also offers a new template system (Otama) that completely separates the presentation logic from the templates. Being written in pure HTML, the template is able to describe the C++ coded logic file. This enables programmers and designers to collaborate.

### Model

In the model layer, called SqlObject, an O/R mapper (O/R mapping system) is provided. App developers will therefore be able to focus on developing business logic without having to write too much because of this SQL.
However, that being said, if data needs to be used in complex conditions, SQL can be readily utilized. Through the use of placeholders, SQL queries may be run easily and safely.
In addition, this framework is compatible with all major database programs such as MySQL, PostgreSQL, SQLite, DB2, and Oracle.

### Qt-based

Qt and the TreeFrog Framework are linked. Not only is the Qt GUI framework powerful, but the non-GUI functionality is also very good. By combining Web applications for the core module container class, network, SQL, JSON, unit test, and meta-object, app developers are provided with very convenient usability features.

### Open source

TreeFrog Framework is open-source software, under the new BSD license (3-clause BSD License).