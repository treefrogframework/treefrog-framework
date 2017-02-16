---
title: Session
page_id: "050.010"
---

## Session

On a Web site, a variety of information will be carried over from one page to another. A familiar example is the shopping cart in which the selected product numbers are carried across the page.

The HTTP protocol does not retain any information (it is said to be stateless). Therefore, some kind of mechanism is necessary in order to retain the information. The mechanism for this is called a **session**; it will be provided by the framework (or language).

 - In addition, in another sense, there is also the case where the Web site visitor refers to a "series of communications" to leave the site.

## Adding Data to the Session

To add data to the session, do the following. Access to the session in the action.

```
 session().insert("name", "foo");
  or
 session().insert("index", 123);
```

Alternatively, we could also write as follows.

```
 session()["name"] = "foo";
  or
 session()["index"] = 123;
``` 

## Reading the Data from the Session

Read the data from the session as follows.

```
 QString name = session().value("name").toString();
  or
 int index = session().value("index").toInt();
```

Alternatively, we could also write as follows.

```
 QString name = session()["name"].toString();
  or
 int index = session()["index"].toInt();
``` 

## Set the Session Destination

As you have seen so far, the session can be considered an “associative array (hash)" with the key value, the data string. The session itself is a single object, but we need somewhere for the saved (persisted) information in order to be able to carry it over between pages.

In TreeFrog Framework, you can select one file, RDB (SqlObject), from cookies as the storage location for a session. This is achieved by using the Session.StoreType setting in application.ini.
 

## Set Cookie to a Location to Save the Session

If you would like to set the cookie destination, you can simply write:

```
 Session.StoreType=cookie
``` 

By saving the cookies so that the contents of the session will be saved to the client (browser) side, the contents will be available for the users to view if you wish to allow it. Information that should not be shown to the user should be saved on the server side (such as RDB). As a rule, you should try to put in the session only the minimum necessary information.

## Set in the File Where to Save the Session

If you would like to set the file destination, you can simply write the following. (Session files will continue to be made in the tmp directory of the application root directory.)

```
 Session.StoreType=file
```

This method is not much use if the AP server is parallelized on multiple machines. Because the request from the user does not always reach the same AP server, a shared file server is required as the destination of the session file. Furthermore, in order to operate properly and safely, a file locking mechanism is needed.

## Set the RDB where to save the session

As a prerequisite, the database information has been set in the *database.ini* file.
In order to save the session to RDB, a table is required. Let's create a 'session' table to a database with an SQL statement such as the following.
Example in MySQL :

```sql
 > CREATE TABLE session (id VARCHAR(50) PRIMARY KEY, data BLOB, updated_at TIMESTAMP);
```

The *application.ini* file should also be edited.

```
 Session.StoreType=sqlobject
```

That's all there is to it. If necessary the system can be allowed to save the rest of the session to the DB.

## Session Lifetime

The validity period of the session, in seconds, is set Session.LifeTime in the configuration file. If the expiration date has passed, the session is erased or destroyed leaving nothing behind. In addition, you can specify 0, which means the session will be valid only while the browser is running. In this case, the session is discarded when the browser is closed.

### Column

One session is assigned for each browser. Each different PC has a different session, and the session is also different if a different browser is used on the same PC.

The Framework keeps track of the sessions by allocating each one with a number.

A unique ID (hard to guess) will be allocated to each session. The ID is stored in the PC browser as a cookie, and sent again aboard the HTTP request, unless the expiration date has been exceeded. The Framework then finds the session from the storage location corresponding to the ID.