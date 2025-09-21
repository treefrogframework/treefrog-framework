---
title: Controller
page_id: "050.0"
---

## Controller

The Controller is the central class of a Web application. It receives a request from the browser, call the business logic with focus on the model, generates HTML to the view on the basis of the results, and returns the response.

## Defining Actions

An **action** is called on the basis of the requested URL which decides about the  method to be called defined in the controller.

Let's add some actions to the "Scaffolding" as it is being generated. First, the part of the 'public slots' is declared in the header file action. Please note that when you declare any other parts, they (by normal methods) will not be recognized. If you want to add arguments to the action, these should be declared in the QString type. Up to 10 arguments may be specified there.

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

After that, carry on implementing those actions in the same way as always.

## Determination of an Action

When the URL string is requested, the correct action to be invoked will be determined. By default, the following rules will be used.

```
 /controller-name/action-name/argument1/argument2/ ...
```

These arguments will be referred as the "Action Arguments".

If more than 10 are specified, only 10 arguments will be taken into account. If you want to use 11 or more arguments, please use the post or data URL arguments as described below.

Here are some concrete examples. Actions from the BlogController class are called in each case.

```
 /blog/show        →  show();
 /blog/show/2      →  show(QString("2"));
 /blog/show/foo/5  →  show(QString("foo"), QString("5"));
```

If the action name is omitted, then the action with the name *index* will be called by default. This could look like as follow:

```
 /blog   →  index();
```

If the action corresponding to the requested URL is not defined, status code 500 (Internal Server Error) is returned to the browser.

For each action that is called, the following process will be carried out in most cases:

* scrutiny of the request
* access the cookies and the session
* access the upload file
* call the model (business logic)
* passing variable(s) to a view
* request the creation of the HTML response

This usually means a lot of work and code to be written for the programmer. Therefore, as a result, some people may end up with a large and complex controllers. To combat this, try to keep the implementation of the model side business logic only. You should always try to keep the controller as simple and as small as possible.

Once you have learned the mechanism on how an action is invoked, it is possible to customize the action that is determined by the URL. Please see the section on [URL routing](http://www.treefrogframework.org/documents/controller/url-routing){:target="_blank"} for more details.

## Acquisition of the Request

An HTTP request is represented by the THttpRequest class.
In the controller, you can obtain ThttpRequest object from the httpRequest() method.

```c++
const THttpRequest & TActionController::httpRequest () const;
```

You can retrieve various data from here.

## Receiving the Request Data

An HTTP request sent from the client (browser) consists of a method, a header, and a body. The incoming data, which arrives the server, is referred to it in the following terms.

* post data - data submitted from a form using the POST method
* URL argument (query parameter) - data assigned to a URL argument after "?" as (format of key = value & …)
* Action argument - data that has been granted after action (the 3 part of "/blog/edit/3") ←see above.

The following example shows how to obtain post data in the controller.
Let's consider that you've made an \<input> tag in the view.

```
 <input type="text" name="title" />
```

To get the value that has been sent from the browser to the controller (server-side), use the following code:

```c++
 QString val = httpRequest().formItemValue("title")
```

By the way, if you want to convert a value into *int*, you can use the Qt included toInt() method.

If you have a large number of data items sent, getting them one by one can be a bit cumbersome. For this, there is also a method available to obtain the values all at once.<br>
For example, you could write your tags as follows.

```
 <input type="text" name="blog[title]" />
 <input type="text" name="blog[body]" />
```

In the controller, data can be obtained as follows:

```c++
 QVariantMap blog = httpRequest().formItems("blog");
 QVariant t = blog["title"];
 QVariant b = blog["body"];
  :
```
Request data can be represented by the hash format.

How to get the URL argument (query parameters)? Here is an example:

```
 http://example.com/blog/index?mode=normal
```

You can use the following method to the value named *mode* here in the *index* action of the *blog* controller.

```
 QString val = httpRequest().queryItemValue("mode");
 // val = "normal"
```

By the way, if you want to get the data without specifying the URL argument and post data, you can use the methods allParameters() and methods().

In order to verify whether requested data is in the form required by the application side, a validation function is provided, too. For more information, visit the chapter on [validation](/en/user-guide/helper-reference/validation.html){:target="_blank"}.

## Passing Variables to a View

To pass variables to a view, use the macros* texport(variable)* or *T_EXPORT(variable)*. You can specify the arguments as *QVariant*, *int*, *QString*, *QList*, or *QHash*. Or you can, of course, also specify the instance of a user-defined model. These are used as follows:

```c++
 QString foo = "Hello world";
 texport(foo);

 // Alternatively

 int bar;
 bar = …
 texport(bar);
```

**Note:** the variable must be specified as an argument to texport. You can't directly specify a string ("Hello world") or numbers (such as 100).

To use a variable inside a view, you must first declare the variable in *tfetch (Type, variable)*. Please see the [view]({{ site.baseurl }}/en/user-guide/view/index.html){:target="_blank"} chapter for more information.

##### In brief: Pass the object to the view by using tfetch().

**In case of an user-defined class:**

When you make the new class passing to the view (user-defined class), please add the following charm at the end of the header class. For more information about this, see the section on ["Creating original model"](/en/user-guide/model/index/html){:target="_blank"}.

```c++
 Q_DECLARE_METATYPE(ClassName)     // ← Please replace the class name
```

You don't need to declare a class that is provided in Qt's Q_DECLARE_METATYPE. However, declaration is required if you want to pass an object of a template class to the view, such as QHash and QList. In that case, please add the following at the end of the *helpers/applicationhelper.h*.

```c++
   :
 Q_DECLARE_METATYPE(QList<float>)
```

A Q_DECLARE_METATYPE macro argument is the name of a class. But if you include a comma (such as QHash \<Foo, Bar\>), a compile error will occur. In this case, the name used should be declared as typedef QHash \<Foo, Bar\>.

```c++
 typedef QHash<Foo, Bar> BarHash;
 Q_DECLARE_METATYPE(BarHash)
```

#### Export object

We refer to an object passed to the view (object that set in the texport() method) as an "export object".

## Requesting the Creation of a Response

After processing the business logic, a result of the process as an HTML response will be returned. If the BlogController *show* action is executed, the response is generated by the template name (extension due to the template system) as in *views/blog/show.xxx*. To request a response, you can use the render() method.

```c++
 bool render(const QString &action = QString(), const QString &layout = QString());
```

If you want to return a response with a different template, specify the action name as an argument. The layout file name can be specified as a layout argument.

The method **render()** means to "request the view/template to create a response". This is also called "drawing".

##### In brief: To render the template, use the render() method.

### Layout

When you create a site, the header and footer and some other parts are usually common to all pages, but with different content. The term layout is used for these common parts on which the template is based. Layout files should be placed in the views/layouts directory.

### About the Template System

TreeFrog, so far, has adopted two template systems: **Otama**, and **ERB**. In ERB, code will be embedded by using <% .. %>. The Otama system has a little different treatment in TreeFrog, because it completely separates the logic (.otm) and the presentation templates (.html).

* ERB uses file extension: xxx.erb
* Otama uses file extensions: xxx.otm and xxx.html<br>(where 'xxx' is the action name)

## Direct Rendering of a String

To render a string directly, use the renderText() method.

```c++
 // "Hello world" render the string
 renderText("Hello world");
```

By default, the layout is not applied. If you want to apply a layout, you must specify *true* as the second argument.

```c++
 // rendering the string "Hello world" while applying a layout
 renderText("Hello world", true);
```

For more information about layouts, please refer to the [view](/en/user-guide/view/index/html){:target="_blank"} chapter.

## Redirect

In order to redirect the browser to another URL, you can use the redirect method. For the first argument, specify the instance of the *QUrl* class.

```c++
 // redirecting to www.example.org
 redirect(QUrl("www.example.org"));
```

You are then able to redirect to other actions on the same host.

```c++
 / redirecting to the incdex action of the Blog controller
 redirect( url("blog", "index") );
```

Here is another useful url method. It returns the appropriate QUrl instance if you pass the controller and action names.

To redirect to other actions inside the same controller, you can omit the controller name.

```c++
 // redirecting to the show action of the same controller
 redirect( urla("show") );
```

It is important to use the urla() method.

## To Display Messages in the Redirect Destination

A redirection leads to another URL. This is because, from the point of view of the server, the URL receives a new destination, thus it has the effect of another action being called.

In the TreeFrog Framework, there is a mechanism to pass a message (via a variable) to the controller to which you have transferred in the redirect. See the following example which passes variables on the tflash() or T_FLASH() method.

```c++
 // foo - the flash object
 QString foo = "successfully";
 tflash( foo );
```

The variable that was passed here is given the name "Flash object".

The flash object is converted to an export object in the view of the redirect. Because of this, it can be output in the echo() or eh() method to display the message.

### The Good Thing About Using the Flash Object

In fact, you can create a web application without using any flash objects. However, if you do use them, when certain conditions are met, code can become an easy-to-understand manner (once you get used to it).

Personally, I think it's better and easier to understand, to have each action completely independent in order to reduce the dependence of actions on each other. I would advise against implementing a call to more than one action in a single request. Let's keep the relationship of one action to one request as much as possible.

If the action creates separate independent displays of almost the same content, the code becomes simpler when using the flash object.

The way the *blogapp* (the subject of the [tutorial chapter](/en/user-guide/tutorial/index/html){:target="_blank"}) uses the *create* and *show* action are good examples. But processing of these actions is different, but both only display the contents of the blog 1 as a result of this processing. In the create action, after the successful data registration, the data will be displayed by redirecting to the show action. At the same time, the message "Created successfully." is displayed by using the flash object.

Actually, an application is not always this simple, because it isn't always necessarily possible to capture and use the flash object. Therefore, please try to use it to strike a balance.

With that being said, in practice most actions come down to either using the *redirect()* or *render()* method.

**By the way …**<br>
In the manner described above, when a redirect occurs, the processing is cut once it goes to the Web application. Objects that survive in spite of this can make other contexts (actions), whereas the flash object is implemented using the session.

## staticInitialize() Method

When starting up, there is only one process in the application. You may want to keep reading the initial data from the DB in advance.

In this case, write the processing as *ApplicationController#staticInitialize()*."

```c++
 void ApplicationController::staticInitialize()
 {
    // do something..
 }
```

This staticInitialize() method, is called only once when the server process is started.

## Lifespan of a controller instance

An instance of a controller is created just before the action is invoked, and destroyed as soon as the action is finished. It means that the creation and the destruction of a controller are associated with each HTTP request.<br>
Because of this specification, the controller doesn't often have instance variables. Please implement the *ApplicationController* the same way.