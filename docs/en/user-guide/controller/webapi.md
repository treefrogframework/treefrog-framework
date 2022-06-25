---
title: Web API
page_id: "050.060"
---

## Web API

Web API is an interface between systems on the Web, i.e., a protocol for passing data between them. By implementing this Web API, data can be passed to and from browsers and other softwares via HTTP or HTTPS.

Web APIs are freely implemented by service providers, and many cloud services offer Web APIs with a REST-style design concept and JSON data format.

## Scaffolding Web API

In TreeFrog, it's easy to generate Web API scaffolds to do CRUD data from a single table.  
For example, the following table exists.

```
sqlite> .schema blog
CREATE TABLE blog (id INTEGER PRIMARY KEY AUTOINCREMENT, title VARCHAR(20), body VARCHAR(200), created_at TIMESTAMP, updated_at TIMESTAMP, lock_revision INTEGER);
```

The following command creates a Web API scaffold for this table.

```
$ tspawn api blog
DriverType:   QSQLITE
DatabaseName: db/dbfile
HostName:
Database opened successfully
  created   models/sqlobjects/blogobject.h
  created   models/objects/blog.h
  created   models/objects/blog.cpp
  updated   models/models.pro
  created   models/apiblogservice.h
  created   models/apiblogservice.cpp
  updated   models/models.pro
  created   controllers/apiblogcontroller.h
  created   controllers/apiblogcontroller.cpp
  updated   controllers/controllers.pro
```

Source files such as controller and service classes have been generated. The data format of the Web API created here is JSON.

Entry points of the Web API are defined in the controller `apiblogcontroller.h` as follows.

```
class T_CONTROLLER_EXPORT ApiBlogController : public ApplicationController {
    Q_OBJECT
public slots:
    void index();                    // Gets all entries
    void get(const QString &id);     // Gets one entry
    void create();                   // New registration
    void save(const QString &id);    // Saves (updates)
    void remove(const QString &id);  // Deletes one entry
};
```

These entry points are as follows.

```
/apiblog/index
/apiblog/get/
/apiblog/create
/apiblog/save/
/apiblog/remove/
```

## Check Web API

Build the source and start the server.

```
 $ make
 $ treefrog -e dev -d
```

Check the operation of the Web API with the curl command.

```
$ curl -sS http://localhost:8800/apiblog/index
{"data":[]}
```

Since no data is registered, empty JSON data is retrieved.

## Add entry point

In addition to the default entry points, additional entry points can be added by writing entries in `config/routes.cfg`. For example, write the followings.

```
# Method   Entry-point               Function

get       /api/blog/index           ApiBlog.index
get       /api/blog/get/:param      ApiBlog.get
post      /api/blog/create          ApiBlog.create
post      /api/blog/save/:param     ApiBlog.save
post      /api/blog/remove/:param   ApiBlog.remove
```

Check that it has been added correctly.

```
$ treefrog --show-routes
Available routes:
  get     /api/blog/index  ->  apiblogcontroller.index()
  get     /api/blog/get/:param  ->  apiblogcontroller.get(id)
  post    /api/blog/create  ->  apiblogcontroller.create()
  post    /api/blog/save/:param  ->  apiblogcontroller.save(id)
  post    /api/blog/remove/:param  ->  apiblogcontroller.remove(id)
```

In addition to GET and POST, PUT and DELETE can be added. See the [URL Routing](/en/user-guide/controller/url-routing.html) page for more information.

Check the operation of the entry point added by the curl command.

```
$ curl -sS http://localhost:8800/api/blog/index    ← Added entry point
{"data":[]}
```

Similarly, empty JSON data could be retrieved.

## Web API for registration

Next, register one item of data. Post JSON data with curl command.

```
$ curl -sS -X POST -H "Content-Type: application/json" -d '{"title":"Hello","body":"hello world"}'  http://localhost:8800/api/blog/create
```

Try to get the list again.

```
$ curl -sS http://localhost:8800/api/blog/index
{"data":[{"body":"hello world","createdAt":"2022-05-25T16:39:02.142","id":1,"lockRevision":1,"title":"Hello","updatedAt":"2022-05-25T16:39:02.142"}]}
```

This time, JSON list data could be retrieved.

Try to get the JSON data by specifying the ID.

```
$ curl -sS http://localhost:8800/api/blog/get/1
{"data":{"body":"hello world","createdAt":"2022-05-25T16:39:02.142","id":1,"lockRevision":1,"title":"Hello","updatedAt":"2022-05-25T16:39:02.142"}}
```

The JSON data could be retrieved successfully.

## Restrict properties to be returned

In the source code after scaffolding, all data in the DB record columns were returned, but here try to restrict the returned data (properties).

Edit the service class `apiblogservice.cpp`.

```
QJsonObject ApiBlogService::index()
{
    auto blogList = Blog::getAll();
    QJsonObject json = { {"data", tfConvertToJsonArray(blogList, {"id", "title", "body"})} };  // ← here
    return json;
}

QJsonObject ApiBlogService::get(int id)
{
    auto blog = Blog::get(id);
    QJsonObject json = { {"data", blog.toJsonObject({"id", "title", "body"})} };  // ← here
    return json;
}
```

After the build, a curl command confirms that only the specified properties were returned.

```
$ curl -sS http://localhost:8800/api/blog/index
{"data":[{"body":"hello world","id":1,"title":"Hello"}]}

$ curl -sS http://localhost:8800/api/blog/get/1
{"data":{"body":"hello world","id":1,"title":"Hello"}}
```

## Summary

We were able to generate simple Web API by scaffolding.

In real cases, this scaffolding-built implementation will not be sufficient. For example, it may retrieve data from two or more tables and return hierarchical JSON data. In such a case, please modify the service class and other classes and implement accordingly.
