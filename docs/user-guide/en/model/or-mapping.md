---
title: O/R Mapping
page_id: "060.010"
---

## O/R Mapping

The model accesses the database internally through O/R mapping objects. This object will basically be a one-to-one relationship, and will be referred to as an ORM object. It can be represented by a diagram such as the following.
One record is related to one object. However, there are often cases of one-to-many relationships, (discussed below).

![ORM](http://www.treefrogframework.org/wp-content/uploads/2011/01/orm.png "ORM")

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

The connection information for the database is specified in the configuration file (*config/database.ini*). The content of the configuration file is divided into three sections: *product*, *test*, and *dev*. By specifying options (-e) in the Web application startup command string (treefrog), you can switch database. In addition, you can do so by adding a section.

Parameters that can be set are as follows.

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

In this way, when you start a Web application, the system will manage the database connection automatically. As the developer, you don’t need to make the process of opening or closing the database.

After you create a table in the database, set the connection information in the dev section of the configuration file; it would help to generate a "scaffolding" in the command generator.

The following sections will be explained using the example BlogObject class which I made in the [tutorial chapter]({{ site.baseurl }}/user-guide/en/tutorial/index.html){:target="_blank"}.

## Reading ORM Object

This is the operation that is the most basic; to find the table with the primary key and then to read the contents of the record.

```c++
int id;
id = ...
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
```

You can also read all of the records.

```c++
TSqlORMapper<BlogObject> mapper;
QList<BlogObject> list = mapper.findAll();
```

You must be careful with this; there is a possibility when the number of record is large, using read all would consume the memory. You can set an upper boundary using the setLimit() method.

The SQL statement is generated inside the ORM. To see what query was issued, please check the [query log]({{ site.baseurl }}/user-guide/en/helper-reference/logging/html){:target="_blank"}.

## Iterator

If you want to process the search results one by one, you can use the iterator.

```c++
TSqlORMapper<BlogObject> mapper;
mapper.find();           // execute queries
TSqlORMapperIterator<BlogObject> i(mapper);
while (i.hasNext()) {       // Itaration
    BlogObject obj = i.next();
    // do something ..
}
```

## Reading ORM Object by Specifying the Search Criteria

The search criteria are specified in the TCriteria class. Importing a single record, "Hello world", into the Title folder is done as follows.

```c++
TCriteria crt(BlogObject::Title, "Hello world");
BlogObject blog = mapper.findFirst(crt);
if ( !blog.isNull() ) {
    // If the record exists
} else {
    // If the record does not exist
}
```

You can also combine multiple conditions.

```c++
// WHERE title = "Hello World" AND create_at > "2011-01-25T13:30:00"
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.add(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // AND add to the end operator
    
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findFirst(crt);
  :
```

If you want to connect with the OR operator conditions, use the addOr() method.

```c++
// WHERE title = "Hello World" OR create_at > "2011-01-25T13:30:00" )
TCriteria crt(BlogObject::Title, tr("Hello World"));
QDateTime dt = QDateTime::fromString("2011-01-25T13:30:00", Qt::ISODate);
crt.addOr(BlogObject::CreatedAt, TSql::GreaterThan, dt);  // OR add to the end operator
  :
```

If you add a condition in addOr() method, conditions clause is enclosed in parentheses. If you use a combination of add() and addOr() methods, take care about the order in which they are called.

<span style="color: #b22222">**NOTE** </span>

Remember, when using AND and OR operators, that the AND operator has priority in being evaluated.

When expressions mix AND and OR operators, the AND operator is evaluated from the expression first, and then the OR operator is evaluated. If you do not wish an operator to be evaluated in this order, it must be enclosed in parentheses.

## Create an ORM object

Make an ORM object in the same way as an ordinary object, and then set the properties. Use the create() method to insert the state in the database.

```c++
BlogObject blog;
blog.id = ...
blog.title = ...
blog.body = ...
blog.create();  // Inserts to DB
``` 

## Update an ORM Object

In order to update an ORM object, you need to create an ORM object that reads the record. You set the properties, and then save the update method.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.title = ...
blog.update();
```

## Delete an ORM Object

Removing ORM object means removing its record as well.
Reads the ORM object and then deletes it by the remove() method.

```c++
TSqlORMapper<BlogObject> mapper;
BlogObject blog = mapper.findByPrimaryKey(id);
blog.remove();
```

As with as other methods, you can remove the records that match the criteria directly, without creating an ORM object.

```c++
// Deletes records that the Title field is "Hello"
TSqlORMapper<BlogObject> mapper;
mapper.removeAll( TCriteria(BlogObject::Title, tr("Hello")) );
```

## Automatic ID Serial Numbering

In some database systems, there is an automatic numbering function for fields. For example, in MySQL, the AUTO_INCREMENT attribute, with the equivalent in PostgreSQL being a field of serial type.
 
The TreeFrog Framework is also equipped with this mechanism. That means that in the examples below numbers are assigned automatically, there is no need to update or to register a new value model. First, create a table with a field for the automatically sequenced number. Then, when the model is created by the generator command, the model is made, without applying updating against that field.

Example in MySQL:

```sql
 CREATE TABLE animal ( id INT PRIMARY KEY AUTO_INCREMENT,  ...
``` 

Example in PostgreSQL:

```sql
 CREATE TABLE animal ( id SERIAL PRIMARY KEY, ...
```

## Automatically Save the Date and Time of Creation and Update

There are often cases where you want to store information such as the date and time and to update the date when the record was created. Since this is a routine implementation, the field names can be set up in accordance with the rules in advance, so that the framework takes care of it automatically.

For saving date and time of creation, you can use the field name called *created_at*. Use the field name *updated_at* instead for saving date and time of any update. We use TIMESTAMP type. If there is such a field, the ORM object records the time stamp at the right time.

<div class="table-div" markdown="1">

| Item                                     | Field Name                |
|------------------------------------------|---------------------------|
| Saving the date and time of creation     | created_at                |
| Saving the date and time of modification | updated_at OR modified_at |

</div><br>

The facility to store the date and time automatically is also in the database itself. My recommendation is that, even though it can be done quite well in the database, it’s better to do it in the framework.

It doesn’t really matter either way, I think either that we do not care, but by following the recommendation to use the framework, you can define elaborate field names as you wish, and leave the database side to do the rest.

## Optimistic Locking

The optimistic locking is a way to save data while verifying that it is not updated by others, without doing “record locking” while updating is taking place. The update is abandoned if another update is already taking place.
 
Prepare a field named lock revision in advance to record, and continue recording, an integer. Lock revision is then incremented with each update. When reading the value and it is found to be different from that in the update, it means that it has been updated from elsewhere. Only if the values are the same, the update processed. In this way, we are able to securely process updates. Since lock is not used, saving of DB system memory and an improvement in processing speeds, even if these are slight, can be expected.

To take advantage of optimistic locking in SqlObject, add an integer type field named lock_revision to the table. It generates a class using the generator. With this alone, optimistic locking is activated when you call TSqlObject::remove() method and TSqlObject::update() method.

<span style="color: #b22222">**In brief: In the table, make a field named lock_revision from type integer.** </span>