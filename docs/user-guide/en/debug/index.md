---
title: Debug
page_id: "110.0"
---

## Debug

When you build the source code that you have created, four shared libraries are generated. These are the substance of the Web application. The TreeFrog application server (AP server) reads them on startup, then waits for access from a browser.

Debugging the Web application is equivalent to debugging the shared common libraries. First of all, let's compile the source code in debug mode. In the application root directory, you can run the following command.

```
 $ qmake -r "CONFIG+=debug"
 $ make clean
 $ make
```

In debug, the following settings are used according to the platforms.

In the case of Linux/ Mac OS X:

<div class="center aligned" markdown="1">

**In the case of Linux/ Mac OS X:**

</div>

<div class="table-div" markdown="1">

| Option                                                | Value                                          |
|-------------------------------------------------------|------------------------------------------------|
| Command                                               | tadpole                                        |
| Command argument                                      | \--debug -e dev (Absolute path of the app root) |
| LD_LIBRARY_PATH env variable<br>(not needed on Mac OS X) | Specify the *lib* directory of web application.  |

</div><br>
 
<div class="center aligned" markdown="1">

**In the case of Windows:**

</div>
<br>
<div class="table-div" markdown="1">

| Option           | Value                                                                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Command          | tadpole**d**.exe                                                                                                                                                                         |
| Command argument | \--debug -e dev (Absolute path of the app root)                                                                                                                                       |
| PATH variable    | Add TreeFrog's bin directory C:\TreeFrog\x.x.x\bin at the beginning. Also, if you use something like MySQL or PostgreSQL, the directoryincluding the client DLL should also be added. |
| TFDIR variable   | The TreeFrog directory is set, c:\TreeFrog\x.x.x.                                                                                                                                    |

</div><br>

- x.x.x is the version of TreeFrog.

Next we will configure these items.
 
## Debugging with Qt Creator

Let’s introduce debugging using the Qt Creator, I think the way you debug is basically the same with other debuggers.

First, make a thread in the [MPM]({{ site.baseurl }}/user-guide/en/performance/index.html){:target="_blank"} application configuration file.

```
 MultiProcessingModule=thread
```

Import the source code of the application file to Qt Creator. Then click [File] – [Open File or Project...] and then choose the project file on the file selection screen. Click the [Configure Project] button, and then import the project. The following screen is seen when the blogapp project is imported.

![QtCreator Import](http://www.treefrogframework.org/wp-content/uploads/2012/12/QtCreator-import.png "QtCreator Import")

Now we will run the settings screen for debugging.
The last of the tadpole command arguments, specifies -e option and the application route’s absolute path. You may remember that the -e option is the setting for switching the DB environment. Let's assume you choose dev.
 
In the case of Linux:<br>
In the next screen we choose /var/tmp/blogapp as the application root.

![QtCreator runenv](http://www.treefrogframework.org/wp-content/uploads/QtCreator-runenv(1).png "QtCreator runenv")
 
In Windows:<br>
We can set the content in two ways by building the configuration screen and by implementing the configuration screen.

Example of build configuration: (sorry for only having Japanese images for this demonstration...)

![QtCreator build settings window 1](http://www.treefrogframework.org/wp-content/uploads/2012/12/QtCreator-build-settings-win.png "QtCreator build settings window 1")

And an example of run configuration :

![QtCreator build settings window 2](http://www.treefrogframework.org/wp-content/uploads/QtCreator-run-settings-win.png "QtCreator build settings window 2")

That is all about the configuration settings.
When adding a breakpoint to the source code, always try to access it from your Web browser.

Check the processing. Does it stop at the breakpoint?