---
title: Debug
page_id: "110.0"
---

## Debug

When you build the source code that you have created, four shared libraries are generated. These are the basement of your Web application. The TreeFrog application server (AP server) reads them during the startup, then waits for access from a browser.

Debugging the Web application is equivalent to debugging the shared common libraries. First of all, let's compile the source code in debug mode. <br>
In the application root directory, you can run the following command:

```
 $ qmake -r "CONFIG+=debug"
 $ make clean
 $ make
```

When debugging, the following settings are used according to the platforms:

<div class="center aligned" markdown="1">

**In the case of Linux/macOS:**

</div>

<div class="table-div" markdown="1">

| Option                                                | Value                                          |
|-------------------------------------------------------|------------------------------------------------|
| Command                                               | tadpole                                        |
| Command argument                                      | \--debug -e dev (Absolute path of the app root) |
| LD_LIBRARY_PATH env variable<br>(not needed on macOS) | Specify the *lib* directory of web application.  |

</div><br>

<div class="center aligned" markdown="1">

**In the case of Windows:**

</div>
<div class="table-div" markdown="1">

| Option           | Value                                                                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Command          | tadpole**d**.exe                                                                                                                                                                         |
| Command argument | \--debug -e dev (Absolute path of the app root)                                                                                                                                       |
| PATH variable    | Add TreeFrog's bin directory C:\TreeFrog\x.x.x\bin at the beginning. Also, if you use something like MySQL or PostgreSQL, the directoryincluding the client DLL should also be added. |
| TFDIR variable   | The TreeFrog directory is set, c:\TreeFrog\x.x.x.                                                                                                                                    |

</div><br>

- x.x.x stands for the version of TreeFrog.

Next we are going to configure these items.

## Debugging with Qt Creator

Let's introduce *debugging* using the *Qt Creator*, I think the way you debug is basically the same with other debuggers.

First, make a thread in the [MPM]({{ site.baseurl }}/en/user-guide/performance/index.html){:target="_blank"} application configuration file.

```
 MultiProcessingModule=thread
```

Import the source code of the application file to the Qt Creator. Then click *[File] – [Open File or Project...]* and then choose the project file on the file selection screen. Click the *[Configure Project]* button, and then import the project. The following screen is seen when the *blogapp* project has been successfully imported.

<div class="img-center" markdown="1">

![QtCreator Import]({{ site.baseurl }}/assets/images/documentation/QtCreator-import.png "QtCreator Import")

</div>

Now we will run the settings screen for debugging.<br>
The last of the tadpole command arguments, specifies the *-e* option and the absolute path of application root. You may remember that the -e option is the option for switching the DB environment. Let's assume you choose *dev*.

In the case of Linux:<br>
In the next screen we choose /var/tmp/blogapp as the application root.

<div class="img-center" markdown="1">

![QtCreator runenv]({{ site.baseurl }}/assets/images/documentation/QtCreator-runenv(1).png "QtCreator runenv")

</div>

In Windows:<br>
We can set the content in two ways by building the configuration screen and by implementing the configuration screen.

Example of a build configuration: (sorry for only having Japanese images for this demonstration...)

<div class="img-center" markdown="1">

![QtCreator build settings window 1]({{ site.baseurl }}/assets/images/documentation/QtCreator-build-settings-win.png "QtCreator build settings window 1")

</div>

And an example of run configuration:

<div class="img-center" markdown="1">

![QtCreator build settings window 2]({{ site.baseurl }}/assets/images/documentation/QtCreator-run-settings-win.png "QtCreator build settings window 2")

</div>

That is all about the configuration settings.<br>
When adding a breakpoint to the source code, always try to access it from your Web browser.

Now check out whether the browser stops at the previously placed breakpoint. Does it stop at the there?

## Debugging WebSocket with Qt Creator

To debug TreeFrog application containing WebSocket, modify Run Configuration of your project:

<div class="center aligned" markdown="1">

**In the case of Linux/macOS:**

</div>
<div class="table-div" markdown="1">

| Option             | Value                                       |
|--------------------|---------------------------------------------|
| Command            | treefrog                                    |
| Command argument   | -e dev (Absolute path of the app root)      |
| Working directory  | %{buildDir}                                 |

</div><br>

<div class="center aligned" markdown="1">

**In the case of Windows:**

</div>
<div class="table-div" markdown="1">

| Option             | Value                                       |
|--------------------|---------------------------------------------|
| Command            | treefrog**d**.exe                           |
| Command argument   | -e dev (Absolute path of the app root)      |
| Working directory  | %{buildDir}                                 |

</div><br>

Now run the application *[Build] – [Run]* and then attach to tadpole process (tadpole**d** on Windows)

To attach to tadpole process in QtCreator click *[Debug] – [Start Debugging] - [Attach to Running Application...]*. It is possible to create a keyboard shortcut for this operation.
