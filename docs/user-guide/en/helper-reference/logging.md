---
title: Logging
page_id: "080.050"
---

## Logging

Your Web application will log four outputs as follow:

<div class="table-div" markdown="1">

| Log          | File Name    | Content                                                                                                                                                                                                                                                                                      |
|--------------|--------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| App log      | app.log      | Logging of Web application. Developers output will be logged here. See below about the output method.                                                                                                                                                                                                    |
| Access log   | access.log   | Logging access from the browser. Including access to a static file.                                                                                                                                                                                                                           |
| TreeFrog log | treefrog.log | Logging of the TreeFrog system. System outputs, such as errors, are logged here.                                                                                                                                                   |
| Query log    | query.log    | Query log issued to the database. Specify the file name in the file value of SqlQueryLog in the configuration file. When stopping the output, flush it. Because there is overhead when the log is outputting. It is a good idea to stop the output when you operate a formal Web application. |

</div><br>

## Output of the Application Log

The application log is used for logging your Web application. There are several types of methods that you can use to output the application log:

* tFatal()
* tError()
* tWarn()
* tInfo()
* tDebug()
* tTrace()

Arguments that can be passed here are the same as the printf-format of format string and a variable number. For example, like this:

```c++
tError("Invalid Parameter : value : %d", value);
```

Then, the following log will be output to the *log/app.log* file:

```
 2011-04-01 21:06:04 ERROR [12345678] Invalid Parameter : value : -1
```

Line feed code is not required at the end of the format string.

## Changing Log Layout

It is possible to change the layout of the log output, by setting FileLogger.Layout parameters in the configuration file *logger.ini*.

```ini
# Specify the layout of FileLogger.
#  %d : date-time
#  %p : priority (lowercase)
#  %P : priority (uppercase)
#  %t : thread ID (dec)
#  %T : thread ID (hex)
#  %i : PID (dec)
#  %I : PID (hex)
#  %m : log message
#  %n : newline code
FileLogger.Layout="%d %5P [%t] %m%n"
```

When a log was generated, date and time will be inserted there and tagged with '%d' in the log layout.<br>
The date format is specified in the FileLogger.DateTimeFormat parameter. The format that can be specified is the same value as the argument of QDateTime::toString(). Please refer to the [Qt document](http://doc.qt.io/qt-5/qdatetime.html){:target="_blank"} for further detail.

```ini
# Specify the date-time format of FileLogger, see also QDateTime
# class reference.
FileLogger.DateTimeFormat="yyyy-MM-dd hh:mm:ss"
```

## Changing the Logging Level

You can set the log output level using the following parameter in *logger.ini*:

```ini
# Outputs the logs of equal or higher priority than this.
FileLogger.Threshold=debug
```

In this example, the log level is higher than debug.

##### In brief: Using the tDebug() function to output the debug log (necessary for development).
