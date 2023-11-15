---
title: Object-Document Mapping in MongoDB
page_id: "060.060"
---

## Object-Document Mapping in MongoDB

MongoDB expresses data to be saved in a JSON-like format and saves it as a document. The function of associating such a document with an object in a programming language is called **object-document mapping (O/D mapping)**.

As in the [O/R mapping](/en/user-guide/model/or-mapping.html){:target="_blank"} chapter described, one document here is also associated with one object in the O/D mapping.

Since MongoDB documents are JSON-like formats, it is possible to have a hierarchical (nested) structure, but in O/D mapping, an object doesn't correspond to two or more levels of documents. For example, the following example shows the only supported simple form in O/D mapping:

```json
{
  "name": "John Smith",
  "age": 20,
  "email": "foo@example.com"
}
```

## Setup

If you are not sure anymore how to setup a MongoDB connection properly, please refer to the [Access MongoDB](/en/user-guide/model/access-mongodb.html){:target="_blank"} chapter.

For generating the class for O/D mapping, execute the following command in the application root directory. In this example, we create a collection named *foo*. The name of the model will be *Foo*.

```
 $ tspawn mm  foo
  created   models/mongoobjects/fooobject.h
  created   models/foo.h
  created   models/foo.cpp
  updated   models/models.pro
```

The next step is to define the data to be stored in the document. Then we edit the c++ header file *models/mongoobjects/fooobject.h* by adding the QString variables *title* and *body*.

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
public:
    QString title;     // ← Add here
    QString body;      // ← Add here
    QString _id;
    QDateTime createdAt;
    QDateTime updatedAt;
    int lockRevision;
    enum PropertyIndex {
        Id = 0,
        CreatedAt,
        UpdatedAt,
        LockRevision,
    };
 :
```

Variables other than  **_id** are not mandatory, so you can delete them. The variable *_id* is equivalent to the ObjectID in MongoDB, therefore please **don't** delete it.

- This object is responsible for accessing MongoDB, which is why we call it "Mongo object" from the time being.

Execute the following command again in order to reflect the added contents to other files.

```
 $ tspawn mm foo
```

Type 'Y' for all files with changes.<br>
This completes the model with [CRUD](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete){:target="_blank"}.

If you want to generate scaffolds including *controllers* and *views*, you can execute the following command instead of 'tspawn mm foo'.

```
 $ tspawn ms foo
```

Now, scaffolding has been generated. After compiling, try running the AP server.

```
 $ treefrog -d -e dev
```

By accessing *http://localhost:8800/foo/* in the browser, a screen which contains a list will be displayed. Use this screen as a starting point for registering, editing and deleting data.

As you can see, this file is similar to the migration file (in Rails), because you automatically define/change the layout of the Mongo document by editing the class of the Mongo object.

## Read a Mongo object

Let's see how a Mongo object, which is referring to the class created by scaffolding, can be read. We do this by loading a Mongo object by using the object ID as a key.

```c++
QString id;
id = ...
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
```

## Create a Mongo object

Make it the same way as instantiating ordinary objects by setting properties. After this is done, calling the *create()* method will create a new document in the MongoDB for you.

```c++
FooObject foo;
foo.title = ...
foo.body = ...
foo.create();
```

Since the object ID is generated automatically, please don't set anything.

## Update a Mongo object

Obtain the Mongo object, for example, by calling its object ID and set a new value. When this is done, the *update()* method will eventually update the object in the MongoDB internally.

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.title = ...
foo.update();
```

There is also a *save()* method for saving the document.<br>
This calls *create()* method **if the corresponding document does not exist** in the MongoDB, and then the *update()* method **if it exists**.

## Delete a Mongo object

Deleting a Mongo object deletes the document. Use the *remove()* method to remove the object.

```c++
TMongoODMapper<FooObject> mapper;
FooObject foo = mapper.findByObjectId(id));
foo.remove();
```

##### Supplement

As mentioned above, Mongo objects can be used in the same way as ORM objects (O/R mapper objects). Looking from the controller's point of view, since the functions provided by the model class are the same, there is no difference in their use. These objects are hidden in the 'private' area of the model.

In other words, if you define model class names which do not overlap, you can even have access to MongoDB and RDB simultaneously, which enables you to easily distribute data to multiple DB systems. If implemented correctly, you can reduce the load of RDB that sometimes may tend to be a bottleneck of the system.<br>
However, when distributing it, you should consider whether the data is supposed to be saved in RDB or in MongoDB depending on the nature of the data. Probably, the question is whether you really want to use transactions or not?

In this way, you can easily access to database systems with different mechanisms which gives you the opportunity to build highly scalable systems as a Web application.

**Differences between the Mongo object class and the ORM object class:**

In a Mongo object class, you can define **QStringList**s as an instance variable like the following sequence of code visualizes:

```c++
class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
public:
    QString _id;
    QStringList  texts;
     :
```

* Please consider: **QStringList** cannot be defined in ORM object classes.