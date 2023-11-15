---
title: Access MongoDB
page_id: "060.050"
---

## Access MongoDB

MongoDB is a document-oriented open source database. It is one of the so-called NoSQL systems.

To manage the data in the RDB, you need to define a table (schema) in advance, but you will most likely not need to do that with MongoDB. In MongoDB, data is represented by a like JSON format (BSON) called "Documents", and the set is managed as a "collection". Each document has assigned a unique ID (ObjectID) by the system.

Here is a comparison between the layered structures of an RDB and MongoDB.

<div class="table-div" markdown="1">

| MongoDB    | RDB      | Remarks       |
|------------|----------|---------------|
| Database   | Database | The same term |
| Collection | Table    |               |
| Document   | Record   |               |

</div><br>

## Installation

Re-install the framework with the following command:

```
 $ tar xvzf treefrog-x.x.x.tar.gz
 $ cd treefrog-x.x.x
 $ ./configure --enable-mongo
 $ cd src
 $ make
 $ sudo make install
 $ cd ../tools
 $ make
 $ sudo make install
```

- x.x.x represents the current version you have downloaded.

## Setting Credentials

Assume that MongoDB has being installed and the server is running. Then you can make a skeleton of the application in the generator.

Let's set the connection information in order to communicate with the MongoDB server. First, edit the following line in *config/application.ini*.

```ini
 MongoDbSettingsFile=mongodb.ini
```

Then edit the *config/mongodb.ini*, specifying the host name and database name. As for the configuration file for the SQL database, this file is divided into three sections, *dev*, *test*, and *product*.

```ini
 [dev]
 DatabaseName=foodb        # Database name
 HostName=192.168.x.x      # Host name or IP address
 Port=
 UserName=
 Password=
 ConnectOptions=           # unused
```

Now, let's check the settings by execute the following command in the application root directory *when MongoDB is running*:

```
 $ tspawn --show-collections
 DatabaseName: foodb
 HostName:     localhost
 MongoDB opened successfully
 -----------------
 Existing collections:
```

If it succeeds, it will be displayed like above.

Web applications can access both MongoDB and SQL databases. This enables the Web application to respond in a flexible way in case of an increased load on the Web system.

## Creating New Document

To access the MongoDB server, use the *TMongoQuery* object. Specify the collection name as an argument to the constructor to create an instance.

MongoDB document is represented by a QVariantMap object. Set the key-value pair for the object and then insert it by using the insert() method into the MongoDB at the end.

```c++
#include <TMongoQuery>
---
TMongoQuery mongo("blog");  // Operations on a blog collection
QVariantMap doc;

doc["title"] = "Hello";
doc["body"] = "Hello world.";
mongo.insert(doc);   // Inserts new
```

Internally, a unique ObjectID is allocated when the insert() method is being called.

### Supplement

As this example shows, there is no need for developers to worry at all about the process of connect/disconnect with MongoDB, because the management of the connection is handled by the framework itself. Through the  mechanism of re-using connections, the overhead caused by the number of connections/disconnections is kept low.

## Reading the Document

When you search for documents and some of them (or all) match the previously set criteria, it is necessary to pass the returned documents (if any) one by one into a QVariantMap. Please be ware, that we have to use QVariantMap here, because the search criteria is expressed as QVariantMap, too.

The following example creates an Criteria object which contains two criteria sets and then being passed as an argument to the find() method. Assuming that there is more than one document that matches the search criteria we use the *while* statement to loop through the list of available documents.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;

criteria["title"] = "foo";  // Set the search criteria
criteria["body"] = "bar";  // Set the search criteria

mongo.find(criteria);    // Run the search
while (mongo.next()) {
    QVariantMap doc = mongo.value(); // Get a document
    // Do something
}
```

- Two criteria are joined by the AND operator.

If you are looking for only one documents that matches the criteria, you can use the findOne() method.

```c++
QVariantMap doc = mongo.findOne(criteria);
```

The following example sets an criteria for a 'num'. Only documents those value 'num' is greater than 10 will match. In order to achieve this, use **$gt** as the comparison operator for your criteria object.

```c++
QVariantMap criteria;
QVariantMap gt;
gt.insert("$gt", 10);
criteria.insert("num", gt);   // Set the search criteria
mongo.find(criteria);    // Run the search
  :
```

**Comparison Operators:**

* **$gt**: Greater than
* **$gte**: Greater than or equal to
* **$lt**: Less than
* **$lte**: Less than or equal to
* **$ne**: Not equal
* **$in**: In
* **$nin**: Not in

### OR Operator

Joins query clauses with a logical OR **$or** operator.

```c++
QVariantMap criteria;
QVariantList orlst;
orlst << c1 << c2 << c3;  // Three criteria
criteria.insert("$or", orlst);
  :
```

As described above, the search condition in *TMongoQuery* is represented by an object from type *QVariantMap*. In MongoDB, the search condition is represented by JSON, so when executing a query, the QVariantMap object will be converted to a JSON object. Therefore you can specify all the operators supported by MongoDB provided that you have described them properly according to their rules. An efficient search will be then possible.

There are more operators provided by MongoDB. Please have a look at the [MongoDB documents](http://docs.mongodb.org/manual/reference/operator/nav-query/){:target="_blank"} for an insight view.

## Updating a Document

We will read a document from the MongoDB server and then update it. As indicated by the update() method, we will update one document that matches the criteria.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["title"] = "foo";             // Set the search criteria
QVariantMap doc = mongo.findOne(criteria);   // Get one
doc["body"] = "bar baz";               // Change the contents of the document

criteria["_id"] = doc["_id"];          // Set ObjectID to the search criteria
mongo.update(criteria, doc);
```

It is important to note here, that even if there are several documents matching the search criteria, but in order to be sure the the document can be updated, add the *ObjectID* to the search criteria.

In addition, if you want to update all documents that match the criteria, you can use the updateMulti() method.

```c++
mongo.updateMulti(criteria, doc);
```

## Removing a Document

Specify the object ID as a condition If you want to delete one document.

```c++
criteria["_id"] = "517b4909c6efa89aed288706";  // Removes by ObjectID
mongo.remove(criteria);
```

You can also remove all documents that match the criteria.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["foo"] = "bar";
mongo.remove(criteria);    // Remove
```
