---
title: Cache
page_id: "080.035"
---

## Cache

In a Web application, every time a user requests, a process such as querying the database and rendering a template is performed to generate HTML data. Many requests have little overhead, but large sites receiving a large number of requests might not be able to ignore them. You don't want to do a lot of work that takes a few seconds, even on a small site. In such a case, the processed data is cached, and the cached data can be reused when the same request is received. Caching leads to a reduction in overhead, and you can expect a quick response.

However, there is also the danger that old data will continue to be showed by reusing cached data. When data is updated, you need to consider how to handle its associated cached data. Do you need to update the cached data immediately, or is it OK to keep showing old data until the timeout expires? Consider the implementation depending on the nature of the data showed.

Be careful when using cache on sites that have different pages for each user, for example, pages that display private information. If you cache page data, cache the data using a different string for each user as a key. If the key strings are not set uniquely, pages of other unrelated users will be displayed. Alternatively, when caching in such cases, it may be safe and effective to limit the data to common data for all users.


## Enable cache module

Uncomment the Cache.SettingsFile in the application.ini file, and set the value of the cache backend parameter 'Cache.Backend'.
```
Cache.SettingsFile=cache.ini

Cache.Backend=sqlite
```
In this example, set up SQLite. You can also configure MongoDB or Redis as other usable backends.

Next, edit cache.ini and set the connection information to the database. By default, it looks like this. Edit as necessary.
```
[sqlite]
DatabaseName=tmp/cachedb
HostName=
Port=
UserName=
Password=
ConnectOptions=
PostOpenStatements=PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA synchronous=NORMAL; VACUUM;
```

If you set up MongoDB or Redis on the backend, set the connection information as appropriate.

## Page caching

Generated HTML data can be cached.

To generate HTML data, the action name was specified for the render () function. To use the page cache, specify two parameters of a key and a time for the renderOnCache () function. To send cached HTML data, use the renderOnCache () function.

As an example, to cache the HTML data of the "index" view with the key "index" for 10 seconds, and to do the cached HTML data, do the following:
```
    if (! renderOnCache("index")) {
          :
          :   // get data..
          :
        renderAndCache("index", 10, "index");
    }
```

If executed by the index action, the third argument "index" can be omitted.
See the [API Reference](http://api-reference.treefrogframework.org/classTActionController.html){:target="_blank"} for details.

## Data caching

You may not want to cache only pages. Binaries and text can be cached.
Use the TCache class to store, retrieve, and delete data.

```
  Tf::cache()->set("key", "value", 10);
    :
    :
  auto data = Tf::cache()->get("key");
    :
```

See [API Reference](http://api-reference.treefrogframework.org/classTCache.html){:target="_blank"} for other methods.