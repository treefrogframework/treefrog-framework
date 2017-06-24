---
title: Generator
page_id: "040.0"
---

## Generator

In this chapter we'll take a look at the generator command with the short name: *tspawn*.

## Generate a Skeleton

First, we must create a skeleton of the application before we can do anything else. We'll use the name *blogapp* again for our creation. Enter the following command from the command line (in Windows, run it from the TreeFrog Command Prompt):

```
 $ tspawn new blogapp
```

When you run this command, the directory tree will then include the application root directory at the top. The configuration file (*ini*) and the project files (*pro*) will be generated, too. The directory is just one of the names you have already seen before.<br>
The following items will be generated as a directory.

* controllers
* models
* views
* heplers
* config      – configuration files
* db          - database file storage (SQLite)
* lib
* log         – log files
* plugin
* public      – static HTML files, images and JavaScript files
* script
* test
* tmp         – temparary directory, such as a file upload

## Generate a Scaffold

The scaffold contains a basic implementation allowing CRUD operations to take place. The scaffold includes the following components: controller, model, source files for *views*, and project files (pro). Therefore, the scaffold forms a good basement on which you can start to establish your full-scale development.

In order to generate a scaffold with the generator command *tspawn*, you need to define a table in the database in advance and to set the database information in the configuration file (*database.ini*).<br>
Now, let's define a table. See the following example:

```sql
> CREATE TABLE blog (id INTEGER PRIMARY KEY, title VARCHAR(20), body VARCHAR(200));
```

If you want to use SQLite for your database, you should make the database file in the application root directory. You can set the database information in the configuration file. The generator command refers to the information that is set in the *dev* section.

```ini
[dev]
driverType=QMYSQL
databaseName=blogdb
hostName=
port=
userName=root
password=root
connectOptions=
```

<div class="center aligned" markdown="1">

**Settings List**

</div>

<div class="table-div" markdown="1">

| Item           | Meaning            | Remarks                                                                                                                                  |
| -------------- |--------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| driverType     | Driver name        | Choices are as follows:<br>- QDB2: IBM DB2<br>- QIBASE: Borland InterBase Driver<br>- QMYSQL: MySQL Driver<br>- QOCI: Oracle Call Interface Driver<br>- QODBC: ODBC Driver<br>- QPSQL: PostgreSQL Driver<br>- QSQLITE: SQLite version 3 or above |
| databaseName   | Database name      | In the case of SQLite a file path must be specified.<br>Example: db/blogdb |
| hostName       | Host name          | *localhost* in the case of blank |
| port           | Port number        | The default port if blank |
| userName       | User name          |     |
| password       | Password           |     |
| connectOptions | Connection options | For more information see Qt documents:<br>[QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"} |

</div><br>

If the database driver is not included in the Qt SDK, you won't be able to access the database. If you haven't built yet, you should setup the driver. Alternatively, you can download the database driver from the [download page](http://www.treefrogframework.org/download){:target="_blank"}, and then install it.

When you run the generator command (after the above mentioned steps), the scaffolding will be generated. Every command should be running from the application root directory.

```
$ cd blogapp
$ tspawn scaffold blog
driverType: QMYSQL
databaseName: blogdb
hostName:
Database open successfully
  created  controllers/blogcontroller.h
  created  controllers/blogcontroller.cpp
  ：
```

<br>
##### In brief: Define the schema in the database and make us the generator command for the scaffolding.

### Relationship of Model-Name/Controller-Name and Table Name

The generator will create class names determined on the basis of the table name. The rules are as follows:

```
　Table name        Model name   Controller name      Sql object name
　blog_entry   →    BlogEntry   BlogEntryController   BlogEntryObject
```

Notice that when the underscore is removed, the next character is capitalized. You may completely ignore any distinction between singular and plural word forms.

## Generator Sub-Commands

Here are the usage rules for the tspawn command:

```
$ tspawn -h
usage: tspawn <subcommand> [args]
Available subcommands:
  new (n)  <application-name>
  scaffold (s)  <model-name>
  controller (c)  <controller-name>
  model (m)  <table-name>
  sqlobject (o)  <table-name>
```

If you specify "controller", "model", or "sqlobject" as a sub-command, you can generate ONLY "controller", "model" and "SqlObject".

### Column

TreeFrog has no migration feature or other mechanism for making changes to and differential management of the DB schema. I think this is unimportant for the following reasons:

1. If I had made a migration function, users would face the extra learning cost.
2. Those that are knowledgeable about SQL can enjoy the full functionality of DB operations.
3. In TreeFrog, it is possible to regenerate only the ORM object classes when changing table.
 → (Unfortunately there might be some possibilities for affecting something to the model class…)
4. I consider that there is little merit to framework-side differential management of SQL commands.

Do you agree?

## Naming Conventions

TreeFrog has a class naming and file naming convention. With the generator, class or file names are generated under the following terms and conditions.

#### Convention for Naming of Controllers

The class name of the controller is "table name + Controller". The controller's class name always begins with an upper-case letter, do not use the underscore ('_') to separate words, but capitalize the first letter (*camelcase*) after where the separator would be.<br>
The following class names are good examples to understand the here described convention:

* BlogController
* EntryCommentController

These files are stored in the controller's directory. File names inside the that folder will be all in lowercase; the class name plus the relevant extension (.cpp or .h).

#### Conventions for Naming Models

In the same manner as with the controller, model names should always begin with a capital letter, erase the underscore ('_') to separate words but capitalize the first letter after where the separator would be. For example, class names such as the following:

* Blog
* EntryComment

These files are stored in the models directory. As well as the controller, the file name will be all in lowercase. The model name is used plus the file extension (.cpp or .h).
Unlike in Rails, we don't use convertion of singular and plural form of words here.

#### View Naming Conventions

Template files are generated with the file name "action name + extension" all in lower case, in the 'views/controller name" directory. The extension, which is used here, depends on the template system.
Also, when you build the view and then output the source file in *views/_src* directory, you will notice that these files have been all converted to C++ code templates. When these files are compiled, a shared library view will be created, too.

#### CRUD

CRUD covers the four major functions found in a Web application. The name comes from taking the initial letters of "Create (generate)," "Read (Read)", "Update (update)", and "Delete (Delete)".
When you create a scaffolding, the generator command generates the naming code as follows:

<div class="center aligned" markdown="1">

**CRUD Correspondence Table**

</div>

<div class="table-div" markdown="1">

|       | Action    | Model     | ORM       | SQL       |
| ----- |-----------|-----------|-----------|-----------|
| C	    | create	| create() [static]<br>create()  | create()	| INSERT    |
| R     | index<br>show | get() [static]<br>getAll() [static] | find() | SELECT |
| U	    | save	    | save()<br>update() | update()	| UPDATE    |
| D	    | remove	| remove()	| remove()	| DELETE    |

</div><br>

## About the T_CONTROLLER_EXPORT Macro

The controller class that you have created in the generator, will have added a macro called T_CONTROLLER_EXPORT.

In Windows, the controller is a single DLL file, but in order to the classes and functions of these available from outside, we need to define it with a keyword __declspec called (dllexport). The T_CONTROLLER_EXPORT macro is then replaced with this keyword. <br>
However, nothing is defined in the T_CONTROLLER_EXPORT in Linux and Mac OS X installations, because a keyword is unnecessary.

```
 #define T_CONTROLLER_EXPORT
```

In this way, the same source code is used and you are thus able to support multiple platforms.