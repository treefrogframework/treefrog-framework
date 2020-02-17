---
title: O/R Mapping
page_id: "060.010"
---

## O/R Mapping

The model accesses the database internally through O/R mapping objects. This object will basically be a one-to-one relationship and will be referred to as an ORM object. It can be represented by a diagram such as the following:<br>

<div class="img-center" markdown="1">

![ORM]({{ site.baseurl }}/assets/images/documentation/orm.png "ORM")

</div>

One record is related to one object. However, there are often cases of one-to-many relationships.<br>
Because the model is a collection of information to be returned to the browser, you need to understand how to access and manipulate the RDB ORM object.

The following DBMS (driver) are supported (this is equivalent to Qt supports):

* MySQL
* PostgreSQL
* SQLite
* ODBC
* Oracle
* DB2
* InterBase

As we proceed with the description, let's check the correspondence of terms between RDB and object-oriented language.

<div class="center aligned" markdown="1">

**Term correspondence table**

</div>

<div class="table-div" markdown="1">

| Object-orientation | RDB    |
|--------------------|--------|
| Class              | Table  |
| Object             | Record |
| Property           | Field  |

</div><br>

## Database Connection Information

The connection information for the database is specified in the configuration file (*config/database.ini*). The content of the configuration file is divided into three sections: *product*, *test*, and *dev*. By specifying options (-e) in the Web application startup command string (treefrog), you can switch the database. In addition, you can do so by adding a section.

Parameters that can be set are the following:

<div class="table-div" markdown="1">

| Parameters     | Description                                                                    |
|----------------|--------------------------------------------------------------------------------|
| driverType     | Driver typeChoose from:<br>QMYSQL, QPSQL, QSQLITE, QODBC, QOCI, QDB2, QIBASE   |
| databaseName   | Database name Specify the file path in the case of SQLite, e.g. *db/dbfile*    |
| hostName       | Host name                                                                      |
| port           | Port                                                                           |
| userName       | User name                                                                      |
| password       | Password                                                                       |
| connectOptions | Connect options<br>Refer to Qt documentation [QSqlDatabase::setConnectOptions()](http://doc.qt.io/qt-5/qsqldatabase.html){:target="_blank"} |

</div><br>

In this way, when you start a Web application, the system will manage the database connection automatically. From developer side, you don't need to deal with the process of opening or closing the database.

After you create a table in the database, set the connection information in the dev section of the configuration file.

The following sections will be using the *BlogObject* class as an example which I made in the [tutorial chapter]({{ site.baseurl }}/en/user-guide/tutorial/index.html){:target="_blank"}.

## Reading ORM Object

This is the most basic operation. It deals with finding documents in a given table. The passed *id* in the findByPrimaryKey() method here is supposed to find a document inside the table which matches with one of the ids (set as primary key). If a match has been decected, the content of the record will be then read.

```c++
int id;
id = ...
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
```

You can also read all records by using the findAll() method.

```c++
TSqlORMapper<BlogObject> mapper;
QList<BlogObject> list = mapper.findAll();
```

You must be careful with this; there is a possibility when the number of record is large, using read all would consume the memory. You can set an upper boundary using the setLimit() method.

The SQL statement is generated inside the ORM. To see what query was issued, please check the [query log]({{ site.baseurl }}/en/user-guide/helper-reference/logging/html){:target="_blank"}.

## Iterator

If you want to deal with the search results one by one, you can use the iterator.

```c++
TSqlORMapper<BlogObject> mapper;
mapper.find();           // execute queries
TSqlORMapperIterator<BlogObject> i(mapper);
while (i.hasNext()) {    // Itaration
    BlogObject obj = i.next();
    // do something ..
}
```

## Reading ORM Object by Specifying the Search Criteria

Search criterias are specified in the TCriteria class. Importing just a single record such as "Hello world" into the *Title* folder can be done in the following manner:

```c++
TCriteria crt(BlogObject::Title, "Hello world");
BlogObject blog = mapper.findFirst(crt);
if ( !blog.isNull() ) {
    // If the record exists
} else {
    // If the record does not exist
}
```

You can also combine and apply multiple conditions.

```c++
// WHERE title = "Hello World" AND create_at > "2011-01-25T13:30:00"
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.add(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // AND add to the end operator

TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findFirst(crt);
  :
```

If you want to create conditions connect by the OR operator, use the addOr() method.

```c++
// WHERE title = "Hello World" OR create_at > "2011-01-25T13:30:00" )
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.addOr(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // OR add to the end operator
  :
```

If you add a condition in the addOr() method, the condition clause is enclosed in parentheses. If you use a combination of add() and addOr() methods, take care about the order in which they are called.

##### NOTE

Remember, that when using AND and OR operators together, the AND operator has priority during its evaluation. That means, when expressions mix AND and OR operators, the AND operator is evaluated from the expression first, while the OR operator is evaluated in the second place. If you don't wish an operator to be evaluated in this order, you have to parantheses.

## Create an ORM Object

Create an ORM object in the same way as an ordinary object and then set its properties. Use the create() method to insert it in the database.

```c++
BlogObject blog;
blog.id = ...
blog.title = ...
blog.body = ...
blog.create();  // Inserts to DB
```

## Update an ORM Object

In order to update an ORM object, you need to create an ORM object first, before its record can be read. Once you have fetched an ORM object from the database, you can set the properties and save its new state with the update() method.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.title = ...
blog.update();
```

## Delete an ORM Object

Removing ORM object means removing its record as well.<br>
Reads the ORM object and then deletes it by the remove() method.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.remove();
```

Like other methods, you can remove the records that match the criteria directly, without creating an ORM object.

```c++
// Deletes records that the Title field is "Hello"
TSqlORMapper<BlogObject> mapper;
mapper.removeAll( TCriteria(BlogObject::Title, tr("Hello")) );
```

## Automatic ID Serial Numbering

In some database systems, there is an automatic numbering function for fields. For example, in MySQL, the AUTO_INCREMENT attribute, with the equivalent in PostgreSQL being a field of serial type.

The TreeFrog Framework is also equipped with this mechanism. That means, in the examples below numbers are assigned automatically. There is no need to update or to register a new value model.<br>
First, create a table with a field for the automatically sequenced number. Then, when the model is created by the generator command, we don't to manually apply updating the field (here 'id') anymore.

Example in MySQL:

```sql
 CREATE TABLE animal ( id INT PRIMARY KEY AUTO_INCREMENT,  ...
```

Example in PostgreSQL:

```sql
 CREATE TABLE animal ( id SERIAL PRIMARY KEY, ...
```

## Automatically Saving og the Date and Time while Creation and Update

There are often cases where you want to store information such as date and time or the date and time when a record has been once created. Since this is a routine implementation, the field names can be set up in accordance with the rules in advance, so that the framework takes care of it by itself automatically.

For saving date and time in case of a record creation, you can use the field name called *created_at*. Use the field name *updated_at* instead for saving date and time when updating a record. We use the TIMESTAMP type here for our purpose. If there is such a field, the ORM object sets a time stamp.

<div class="table-div" markdown="1">

| Item                                     | Field Name                |
|------------------------------------------|---------------------------|
| Saving the date and time of creation     | created_at                |
| Saving the date and time of modification | updated_at OR modified_at |

</div><br>

The tools for storing date and time automatically are handeled by the database itself as well. My recommendation is that, even though it can be done quite well in the database, it's better to do it in the framework.

It doesn't really matter either way, I think either that we do not care, but by using the framework, you can define elaborate field names as you wish and leave the database side to do the rest.

## Optimistic Locking

The optimistic locking is a way to save data while verifying that it is not updated by others, without doing "record locking" while updating is taking place. The update is abandoned if another update is already taking place.

In advance, prepare a field named *lock_revision* as an integer to record also using the auto increment ability. Lock revision is then incremented with each update. When reading the value and it is found to be different from that in the update, it means that it has been updated from elsewhere. Only if the values are the same, the update proceeds. In this way, we are able to securely proceeds updates. Since lock is not used, saving of DB system memory and an improvement in processing speeds, even if these are slight, can be expected.

To take advantage of optimistic locking in the SqlObject, add an integer type field named *lock_revision* to the table. It creates a class using the generator. With this alone, optimistic locking is activated when you call TSqlObject::remove() method and TSqlObject::update() method.

##### In brief: Create a field named lock_revision from type integer in the table.
