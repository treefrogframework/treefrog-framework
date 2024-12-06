---
title: URL Routing
page_id: "050.030"
---

## URL Routing

The URL Routing is the mechanism that determines the action to call for a requested URL. When a request is received from a browser, the URL checks for correspondence with the routing rules and, where applicable, the defined action will be called. If no action has been specifically determined, action by the default rule is invoked.

Here is a little reminder of those default rules:

```
 /controller-name/action-name/argument1/argument2/...
```

Let's look at customizing the routing rules.<br>
The routing definition is written in the *config/routes.cfg* file. For each entry, the directives, path and an action are written side by side on a single line. The directive is to select *match*, *get*, *post*, *put* or *delete*.
In addition, a line that begins with '#' is considered to be a comment line.

Here's an example:

```
 match  /index  Merge.index
```

In this case, if the browser requests '/index' either by POST method or GET method, the controller will respond with the *index* action of the *Merge* controller.

The next case is where the get directives have been defined:

```
 get  /index  Merge.index
```

In this case, routing will be carried out only when the '/index' is requested with the GET method. If the request is made by the POST method, it will be rejected.

Similarly, if you specify a post directive, it is only valid for POST method requests. Request in GET method will be rejected.

```
 post  /index  Merge.index
```

The following is about how to pass arguments to the action. Suppose you have defined the following entries as routing rules:

```
 get  /search/:params  Searcher.search
```

It's important to use the keyword ':params'.<br>
In the case of */search/foo*, when the request is made with the GET method, a search action with one argument is called to the *Searcher* controller. The "foo" in the argument is passed.
Similarly with /search/foo/bar. Following this request, a search action with two arguments ("foo" and "bar") is called.

```
 /search/foo    ->   Call search("foo") of SearcherController
 /search/foo/bar ->  Call search("foo", "bar") of SearcherController
```

## Show Routing

After building the app, the following command shows the current routing information.
```
 $ treefrog --show-routes
 Available controllers:
   match   /blog/index  ->  blogcontroller.index()
   match   /blog/show/:param  ->  blogcontroller.show(id)
   match   /blog/create  ->  blogcontroller.create()
   match   /blog/save/:param  ->  blogcontroller.save(id)
   match   /blog/remove/:param  ->  blogcontroller.remove(id)
```
