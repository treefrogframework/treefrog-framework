---
title: Cooperation with the Reserve Proxy Server
page_id: "140.0"
---

## Cooperation with the Reverse Proxy Server

The TreeFrog Framework provides an application server (AP server) to send back the dynamic content at the request of the individual. As previously mentioned, this application server also functions as a Web server, so you can also process static content. For the Web server role, we have specialized in one function only, the implementation of high speed return of static content.

As it stands, this is sufficient for small and medium-sized Web sites. However, when making a large Web site system, in order to achieve stable operation and load balance it becomes necessary to use a Web server (reverse proxy) system such as nginx or Apache.
Also, if you want to work with compression response and SSL, you must set up a Web server separately.

<span style="color: #b22222">**In brief: For large scale sites, set up the web server.** </span>

<div class="center aligned" markdown="1">

**Roles of server-side**

</div>

<div class="table-div" markdown="1">

| Server                     | Role                                                                              |
|----------------------------|-----------------------------------------------------------------------------------|
| Web server (Reverse proxy) | Load balancing, encryption, compression, caching,sending static contents and etc. |
| AP server (TreeFrog)       | Generating and sending dynamic contents (static contents)                         |
| DB server (RDB)            | Data store, persistence                                                           |

</div><br>

I'll not go into detail about reverse proxy configuration for Apache and nginx, since a great deal of information is readily available on the internet. The basic idea is that reverse proxy listens on port 80, with requests being transferred to the application server as they are received. Of course, the application server should be assigned a port number that does not duplicate other services. Use the *ListenPort* parameter in the *application.ini* file to set the port number.

If you run your application server and reverse proxy on the same host, you can use the UNIX domain socket connection to them. The advantages of using the UNIX domain socket are that its overhead is less than the TCP socket and that it cannot be connected to from an external host. That can make you feel a little bit more secure.

For settings corresponding to the UNIX domain socket server application, perform the ListenPort parameters as follows.

```
 ListenPort=unix:/tmp/foo
```

- Please change the file name as necessary.
 
For example, in order to make a reverse proxy to a UNIX domain socket in nginx, add the following entry:

```
upstream backend {
    server unix:/tmp/foo;
}
server {
    listen 80;
    server_name localhost;
    location / {
        proxy_pass        http://backend;
    }
}
```
 
Then all you should need to do is to save the setting.
Start the AP server and then the Web server, then try to visit from the browser to be sure if it is working correctly. If it doesn’t work properly, try looking for the reason and checking the access log.