---
title: Access MongoDB
page_id: "060.050"
---

## Access MongoDB

MongoDB is a document-oriented open source database, it’s one of the so-called NoSQL systems.

To manage the data in the RDB, you need to define the table (schema) in advance, but you will most likely not need to do that with MongoDB. In MongoDB, data is represented by a (BSON) format like JSON called "Documents", and the set is managed as a "collection". Each document is assigned a unique ID (ObjectID) by the system.

Here is a comparison between the layered structures of an RDB and MongoDB.

<div class="table-div" markdown="1">

| MongoDB    | RDB      | Remarks       |
|------------|----------|---------------|
| Database   | Database | The same term |
| Collection | Table    |               |
| Document   | Record   |               |

</div><br>

TreeFrog Framework has been designed to respond to MongoDB in its current trial version. Please first understand that what has been described here may therefore change in future versions

## Installation

Re-install the framework with the following command.

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

- x.x.x will be version.
- create both release and debug versions for Windows.

## Setting Credentials

Assume that MongoDB has being installed, and the server is running. Then you make a skeleton of the application in the generator.

Let’s set the connection information in order to communicate with the MongoDB server. First, edit the following line in *config/application.ini*.

```
 MongoDbSettingsFile=mongodb.ini
``` 
 
Then edit the *config/mongodb.ini*, specifying the host name and database name. As with the configuration file for the SQL database, this file is divided into three sections, *dev*, *test*, and *product*.
 
```
 [dev]
 DatabaseName=foodb        # Database name
 HostName=192.168.x.x      # Host name or IP address
 Port=
 UserName=
 Password=                    
 ConnectOptions=           # unused
```

Check the settings. Execute the following in the application root directory when MongoDB is running.

```
 $ tspawn --show-collections
 DatabaseName: foodb
 HostName:     localhost
 MongoDB opened successfully
 -----------------
 Existing collections:
```

## Creating New Document

To access the MongoDB server, use the TMongoQuery object. Specify the collection name as an argument to the constructor to create an instance.

MongoDB document is represented by QVariantMap object. Set the key-value pair for the object and then insert it by using the insert() method into the MongoDB at the end.

```c++
#include <TMongoQuery>
---  
TMongoQuery mongo("blog");  // Operations on a blog collection
QVariantMap doc;

doc["title"] = "Hello";
doc["body"] = "Hello world.";
mongo.insert(doc);   // Inserts new
```

Internally, a unique ObjectID is allocated when insert() is run.

### Supplement

As this example shows, there is no need for developers to worry at all about the process of connect/disconnect with MongoDB; management of the connection is handled by the framework. Through the mechanism of re-using connections as they become available, the overhead caused by the number of connections/disconnections is kept low.
 
## Reading the Document

Search for the documents that match the criteria; load them one by one. Please take care, since the search criteria are expressed in QVariantMap object as well.

In the following example, we use an object that sets criteria as an argument to the find() method, and then click Search. Assuming that there is more than one document that matches the search criteria we use a while statement to create a loop.

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
 
- Two criteria are joined by AND operator.

To read only one search result, you can use the findOne() method.

```c++
QVariantMap doc = mongo.findOne(criteria);
``` 

The following is a example to read documents that the value of 'num' key is greater than 10. Specify **$gt** as the comparison operator for a criteria object.

```
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

**OR Operator**

Joins query clauses with a logical OR **$or**.

```c++
QVariantMap criteria;
QVariantList orlst;
orlst << c1 << c2 << c3;  // Three criteria
criteria.insert("$or", orlst);
  :
```

Other operators are provided by MongoDB. Please see [MongoDB documents](http://docs.mongodb.org/manual/reference/operator/nav-query/){:target="_blank"} for details.

## Updating the Document

We will read a document from the MongoDB server and then update it. As indicated by the update() method we will update one document that matches the criteria.

```c++
TMongoQuery mongo("blog"); 
QVariantMap criteria;
criteria["title"] = "foo";    // Set the search criteria
QVariantMap doc = findOne(criteria);   // Get one
doc["body"] = "bar baz";      // Change the contents of the document

criteria["_id"] = doc["_id"]; // Set ObjectID to the search criteria  
mongo.update(criteria, doc);
```

As an additional comment, even if there are several documents matching the search criteria, in order to be sure the document can be updated, add ObjectID to the search criteria.

In addition, if you want to update all documents that match the criteria, you can use the updateMulti() method.

```c++
mongo.updateMulti(criteria, doc);
```

## Removing a Document

You can remove all documents that match the criteria.

```c++
TMongoQuery mongo("blog");
QVariantMap criteria;
criteria["foo"] = "bar";
mongo.remove(criteria);    // Remove
```

Specify the object ID as a condition If you want to delete one document.

```c++
criteria["_id"] = "517b4909c6efa89aed288706";  // Removes by ObjectID
mongo.remove(criteria);
```