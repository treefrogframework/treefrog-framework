---
title: Model
page_id: "060.0"
---

## Model

The model is an object that represents the (abstract) information that should be returned to the browser. In fact it is not so simple in terms of business logic. So, let's try to understand it.

Models are stored in a system or in an external database in a persistent state. Looking from the controller side, regardless of the model, you are accessing a database for information wanted for the HTTP response. Once the information has been left in a persistent state in the database, you will be able to get a model with information that you can pass to the view.

When you write directly to SQL to access the database, coding tends to become complicated in some cases and therefore more difficult to read. This is even truer if the DB schema becomes complicated. In order to mitigate this is difficulty, TreeFrog is equipped with an O/R mapping system (named SqlObject).

In a Web application, CRUD (create, read, update, delete) is a set of functions with minimum requirements; the SQL statement you write is almost routine. For this part, the O/R mapping system will be working here very effective.

I personally think, as for the Web frameworks, it is recommended to have the model object itself related to an object for O/R mapping (these objects will be referred as ORM objects). By default, each model object in TreeFrog Framework includes ORM objects.

   "A model has ORM object(s)."

Doing it this way is beneficial in a number of ways:

* Intuitive structure of the ORM class.
* Possible to conceal the information that is not required to show on the controller side.
* Can absorb both logical delete and physical delete of the record as a model.
 → Use this page to create a deletion flag column, it is then only necessary to update it in the model remove() method.
* Since there is no need for a relationship table since the model is one-to-one, the degree of freedom in design model class is greater.
 → Business logic can be added to the model in a natural way.

The disadvantage is that the amount of code increases a bit.

##### In brief: Hide unnecessary information to the controller and the view.

## API of Model

In general, when classes are more independent they are also more reusable. It would therefore be desirable to let dependency of the model to be as small as possible.

In Web applications, the DB is often used for store data in a persisting state, so that "model class" relates to "table". Web applications that must handle many kinds of data have to create many tables (models) accordingly. Because it is common when designing a DB schema to conduct data normalization, models will have a relationship with each other through their properties (fields).

When coding the class of models, the following conventions are in place. These should be learnt.

Use the texport() method to pass arguments to the view.
This is equal to set to a variable from type QVariant (by using the setValue() method) in the following classes:　

    - public default constructor
    - public copy constructor
    - public destructor
    - Declaration in Q_DECLARE_METATYPE macro (At the end of the header per file)

Please read the [Qt documentation](http://doc.qt.io/qt-5/qmetatype.html){:target="_blank"} if you want to learn more about it.

##### ★ A model that is created by the generator command already meets the minimum requirements to work.

The model class that is created in the generator is inherited from the TAbstractModel class. I have inherited it in order to take advantage of its features. Additionally, convenient methods for handling ORM objects will be available, too. Basically, the inheritance is merely for reusability. Those models that don't access the database at all, don't need to deal with inheritance though.

##### In brief: if you want to use an ORM object, you should inherit from the TAbstractModel class regardlessly.

When the model is created by the generator, the getter/setter of each property and the class methods, that are equivalent to "create" and "read", are defined. The following example is an excerpt of the Blog class which we made in the [tutorial chapter]({{ site.baseurl }}/en/user-guide/tutorial/index.html){:target="_blank"}.

```c++
static Blog create(const QString &title, const QString &body);
static Blog create(const QVariantMap &values);
static Blog get(int id);       // Get the model object with a specified ID
static Blog get(int id, int lockRevision);
static QList<Blog> getAll();   // Get all model objects
```

When you run the create() method, the content of the object is stored in the database during its creation.

Let's also look at the methods defined in TAbstractModel class:

```c++
virtual bool create();          // New
virtual bool save();            // Save (New or Updated)
virtual bool update();          // Update
virtual bool remove();          // Remove
virtual bool isNull() const;    // Whether present in the DB
virtual bool isNew() const;     // Whether before saving to DB
virtual bool isSaved() const;   // Whether stored in the DB
void setProperties(const QVariantMap &properties);
```

The save() method internally calls the create() method if the ORM object doesn't already exist, or the update() method if it does exist. So, if you don't want to distinguish between the create() and update() method, then you can simply use the save() method to call the model.

The code which is generated here is only the tip of the iceberg. You can add or modify the property by shifting, for example, from *protected* to *private* or whatever you like.

## Creating a Model with a Different Name to the Table Name

When you create a model using the generator command, the model name are derived from the table name in this format '_' (underscore).<br>
If you want to give an individual model a different name with this format, you can deal with it using the command with a string at the end such as follows:

```
 $ tspawn  model  blog_entries  BlogEntry    ← only model created
 $ tspawn  s  blog_entries  BlogEntry   ←  model-view-controller created
```

## Creating an Original Model

You don't necessarily have to associate a model with the table. It can also be used to summarize relevant data in the case of passing information to the view.

If you want to create a model on its own without the use of a generator, you should declare a class as shown in the following example:

```c++
class T_MODEL_EXPORT Post
{
  public:
    // include default constructor, copy constructor, destructor
    // write the code freely afterward.
};
Q_DECLARE_METATYPE(Post)        // charm to pass to view
Q_DECLARE_METATYPE(QList<Post>) // charm to pass the list to view
```

Save it in the models directory, add files to the project (*models.pro*), and specify the file name of the source and header.
