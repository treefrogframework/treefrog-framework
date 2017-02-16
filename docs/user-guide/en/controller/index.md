---
title: Controller
page_id: "050.0"
---

## Controller

Controller is the central class of the Web application. It receives the request from the browser, call the business logic with focus on the model, generates HTML to view on the basis of the results, and returns the response.

## Defining Actions

An **action** is called on the basis of the requested URL, the method being defined by the controller.

Let's add some action to the "Scaffolding" as it is being generated. First, the part of the 'public slots' is declared in the header file action. Please note that when you declare any other parts, they (by normal methods) will not be recognized. If you want to add arguments to the action, these should be declared in the QString type. Up to 10 arguments may be specified.

```c++
class T_CONTROLLER_EXPORT FooController : public ApplicationController
{
    Q_OBJECT
     :
public slots:   // The action is defined here
    void bar();  
    void baz(const QString &str);
     :
```

After that, carry on implementing in the same way as always.
 
## Determination of the Action

When the URL string is requested, the correct action to be invoked will be determined. By default, the following rules will be used.

```
 /controller-name/action-name/argument1/argument2/ ...
```
 
These arguments will be referred as the "Action Arguments".

If more than 10 are specified it will be assuming that only 10 are to be designated. If you want to use 11 or more arguments, please use the post or data URL arguments as described below.
 
Here are some concrete examples. Actions from the BlogController class are called in each case.

```
 /blog/show      →  show();
 /blog/show/2    →  show(QString("2"));
 /blog/show/foo/5  →  show(QString("foo"), QString("5"));  
```

If the action name is omitted, then index action will be called by default. In other words, like this.

```
 /blog   →  index();
```  
 
If the action corresponding to the requested URL is not defined, status code 500 (Internal Server Error) is returned to the browser.

For each action that is called, the following process will be carried out in most cases;

* scrutiny of the request
* access the cookies and session
* access the upload file
* call model (business logic)
* passing variable to a view
* request the creation of the HTML response

A lot of processing of actions is packed inside the programmer. As a result, some people may end up with a large and complex controller. To combat this, try to keep the implementation of the model side business logic only. You should always try to keep the controller as simple and as small as possible.

Once you have learned about the mechanism by which an action is invoked, it is possible to customize the action to be determined by the URL. Please see the section on [URL routing](http://www.treefrogframework.org/documents/controller/url-routing){:target="_blank"} below.
 
## Acquisition of the Request

An HTTP request is represented by the THttpRequest class.
In the controller, you can obtain ThttpRequest object from the httpRequest() method.

```c++
const THttpRequest & TActionController::httpRequest () const;
```

You can retrieve various data from here.

## Receiving the Request Data

An HTTP request sent from the client (browser) consists of a method, a header, and a body. Incoming data which is sent to be included is referred to in the following terms.

* post data - data submitted from a form using the POST method
* URL argument (query parameter) - data assigned to a URL argument after "?" as (format of key = value & …)
* Action argument - data that has been granted after action (the 3 part of “/blog/edit/3”) ←see above.
 
The following example shows how to start to obtain post data in the controller.
Let’s consider that you’ve made an input tag in the view. 

```html
 <input type="text" name="title" />
```

To get the value of the destination sent to the controller side , do the following.

```c++
 QString val = httpRequest().formItemValue("title")
```

By the way, if you want to convert to type int val, you can use the Qt included toInt() method.

If you have a large number of data items sent, getting them one by one can be a bit cumbersome. There is also a method to obtain the values all at once.
For example, you can write your tags as follows.

```html
 <input type="text" name="blog[title]" />
 <input type="text" name="blog[body]" />
```

From the controller, data can be obtained as follows. Request data can be represented by the hash format.

```c++
 QVariantMap blog = httpRequest().formItems("blog");
 QVariant t = blog["title"];
 QVariant b = blog["body"];
  :
``` 
 
How to get the URL argument (query parameters).
Example:

```
 http://example.com/blog/index?mode=normal
```

You can use the following method to the value of *mode* in *index* action of *blog* controller.

```
 QString val = httpRequest().queryItemValue("mode");
 // val = "normal"
```

By the way, if you want to get the data without specifying the URL argument and post data, you can use the allParameters() and methods() method parameter.
 
In order to verify whether requested data is in the form required by the application side, validation function is provided. For more information, see the chapter on [validation](/user-guide/en/helper-reference/validation.html){:target="_blank"}.
 
## Passing Variables to a View

To pass variables to view, use the macros texport(variable) or T_EXPORT(variable). The argument you can specify for all variables can be QVariant type, int, QString, QList, or QHash. Or you can, of course, also specify the instance of a user-defined model. These are used as follows.

```c++
 QString foo = "Hello world";
 texport(foo);

    // Alternatively

 int bar;
 bar = …
 texport(bar);
```

**Note:** the variable must be specified as an argument to texport. You can’t directly specify a string ("Hello world") or numbers (such as 100).

To use the variable in view, you must first declare the variable in tfetch (Type, variable). Please see the view chapter for more information.

<span style="color: #b22222">**In brief: Pass the object to the view by tfetch().** </span>
 
### In the case of a user-defined class:

When you make the new class passing to the view (user-defined class), please add the following charm at the end of the header class. See the section on ["Creating original model"](/user-guide/en/model/index/html){:target="_blank"}.

```c++
  Q_DECLARE_METATYPE(ClassName)     // ← Please replace the class name
```

You do not need to declare a class that is provided in Qt to Q_DECLARE_METATYPE. However, declaration is required if you want to pass to the view an object of a template class, such as QHash and QList. In that case, please add the following at the end of the *helpers/applicationhelper.h*.

```c++
    :
  Q_DECLARE_METATYPE(QList<float>)
```

A Q_DECLARE_METATYPE macro argument is the name of a class, however, if you include a comma (such as QHash \<Foo, Bar\>), a compile error will occur. In this case, the name used should be declared as typedef QHash \<Foo, Bar\>.

```c++
  typedef QHash<Foo, Bar> BarHash;
  Q_DECLARE_METATYPE(BarHash)
``` 

Export object
We refer to an object passed to the view (object that we texport) as an "export object".
 
## Requesting the Creation of a Response

After processing the business logic returns the result of the process as an HTML response. If the BlogController show action is executed, the response is generated by the template name (extension due to the template system) as in *views/blog/show.xxx*. To request it, you can use the render method.

```c++
 bool render(const QString &action = QString(), const QString &layout = QString());
```

If you want to return a response with a different template, specify the action name as an argument action. The layout file name can be specified as a layout argument.

<span style="color: #b22222">**In brief: To render the template, use render().** </span>

### Layout

When you create a site, the header and footer and some other parts are usually common to all pages, but with different content. The term layout is used for these common parts on which the template is based. Layout files should be placed in the views/layouts directory.

### About the Template System

TreeFrog, has so far adopted two template systems, Otama, and ERB. In ERB code is embedded, as you will know using <% .. %>. The Otama system is specific to TreeFrog and is a template system that completely separates the logic (.otm), and the presentation templates (.html). 
 
* ERB uses file extension: xxx.erb
* Otama uses file extensions: xxx.otm and xxx.html

     (xxx being the action name)
 
## Direct Rendering of a String

To render a string directly, use the renderText method.

```c++
 // "Hello world" render the string
 renderText("Hello world");
```

By default, the layout is not applied. If you want to apply a layout, you must specify *true* as the second argument.

```c++
 // rendering the string "Hello world" while applying a layout
 renderText("Hello world", true);
```

For more information about layouts, refer to the [view](/user-guide/en/view/index/html){:target="_blank"} chapter.

## Redirect

In order to redirect the browser to another URL, you can use the redirect method. For the first argument, specify the instance of QUrl class.

```c++
 // redirecting to www.example.org 
 redirect(QUrl("www.example.org"));
```

You are then able to redirect to other actions on the same host.

```
 / redirecting to the incdex action of the Blog controller 
 redirect( url("blog", "index") );
```

Here is another useful url method. It returns the appropriate QUrl instance if you pass the controller and action names.

To redirect to other actions in the same controller, you can omit the controller name.

```
 // redirecting to the show action of the same controller
 redirect( urla("show") );
```

It is important to use the urla() method.
 
## To Display Messages in the Redirect Destination

When redirection occurs, it is to another URL. This is because, from the point of view of the server, the URL receives a new destination, thus it has the effect of another action being called.

In the TreeFrog Framework, there is a mechanism to pass a message (variable) to the controller to which you have transferred in the redirect. See the following example which passes variables on tflash() method or T_FLASH() method.

```c++
 // foo - the flash object
 QString foo = "successfully";
 tflash( foo );
```

The variable that was passed here is given the name "Flash object".

The flash object is converted to an export object in view of the redirect. Because of this, it can be output in the echo() method or eh() method to then display the message.
 

### The Good Thing About Using the Flash Object

In fact, you can create a web application without using any flash objects. However, if you do use a flash object, when certain conditions are met, code is an easy-to-understand manner (once you get used to it).

Personally, I think it’s better, and easier to understand, to have each action complete and independent in itself. That is, to reduce the dependence of actions on each other. I would advise against implementing a call to more than one action in a single request. Let's keep as much as possible to the relationship of one action to one request.

If the action creates separate independent displays of almost the same content, the code is made simpler by using the flash object.

The way the blogapp (the subject of the [tutorial chapter](/user-guide/en/tutorial/index/html){:target="_blank"}) uses create action and show action are good examples. Processing of these actions is different, but both only display the contents of the blog 1 as a result of the processing. In the create action, after successful data registration, the data is displayed by redirecting to the show action. At the same time, the message "Created successfully." is displayed by using a flash object. 

Actual application is not this simple, it isn’t always necessarily possible to capture and use the flash object. Therefore, please try to use it to strike a balance.

With that said, in practice most actions come down to either *redirect()* method or *render()* method.

By the way …<br>
In the manner described above, when a redirect occurs, the processing is cut once it goes to the Web application. Objects that survive in spite of this can make other contexts (action), whereas the flash object is implemented using the session.
 
## staticInitialize() Method

When starting up, there is only one processing in the application. You may want to keep reading the initial data from the DB in advance.

In this case, write the processing as *ApplicationController#staticInitialize()*.”

```c++
 void ApplicationController::staticInitialize()
 {
    // do something..
 }
```

This staticInitialize() method, is called only once when the server process is started. However, care must be taken so that it does not create an extra load function, by being called every time the process starts when you select the PreFork as [MPM](/user-guide/en/performance/index/html){:target="_blank"}).