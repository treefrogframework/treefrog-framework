# ChangeLog

## 2.1.0
 - Fix a bug of calculating length of receiving packets.
 - Added echo() function with a variantmap argument.
 - Implement the logic of the tspawn command to generate scaffolding for vue.js.

## 2.0.2
 - Updated the scaffold generator to generate WebAPI codes.
 - Modified the scaffold generator to generate service classes for the model layer.

## 2.0.1
 - Support for Qt version 6.
 - Modified not to use obsolete functions of Qt.

## 1.31.0
 - Fix a bug of TMultiplexingServer (epoll server).
 - Modified not to use obsolete functions of Qt.
 - Modified to use std::call_once() function.
 - Added TAbstractSqlORMapper class.
 - Performance improvement.

## 1.30.0
 - Implemented logics for X-Forwarded-For Header.
 - Implemented ActionMailer.smtp.RequireTLS parameter in application.ini.
 - Added a option for showing URL routing to treefrog command.
 - Updated I/F of ORM functions.
 - Performance improvement.

## 1.29.0
 - Fix a bug of max-age of cookie.
 - Fix a bug of generating select-tag.
 - Modified to initialize boolean fields in classes generated.
 - Implemented publish() function in TActionController class.
 - Rename functions; renderAndStoreInCache() to renderAndCache(),
   renderFromCache() to renderOnCache(), removeFromCache() to removeCache().

## 1.28.0
 - Implemented to add a SameSite attribute to cookie.
 - Modified to add a max-age value to cookie.
 - Fix a bug of listing available controllers.
 - Fix a bug of showing a port number by -l option.
 - Fix a bug of content type in renderText() function.

## 1.27.0
 - Implemented OAuth2 client.  [Experimental]
 - Supports for MongoDB version 3.2 and later.
 - Fix a bug that timer for TSqlDatabasePool stops. #279
 - Fix a bug not to set all databases to transaction state value.
 - Fix a bug of execution of diff in tspawn command.
 - Replaced QDir::separator() by "/". #264
 - Added a value, svg:image/svg+xml.

## 1.26.0
 - Added cache modules of SQLite, MongoDB and Redis.
 - Updated LZ4 compression algorithm to 1.9.2.
 - Fix a compilation error on Ubuntu 19.10.
 - Changed the epoll module from multi-thread to single thread architecture.
 - Performance improvement.

## 1.25.0
 - Updated Mongo C driver to v1.9.5.
 - Updated LZ4 compression algorithm to v1.9.1.
 - Modified to set a domain in session cookie. #245
 - Changed the default C++ config from c++11 to c++14.
 - Erased 'DISTINCT' from SELECT statement.
 - Fix a bug of stopping tadpole.
 - Fix a bug of outputting accesslog.
 - Modified for MSVC2015.
 - Performance improvement.

## 1.24.0
 - Modified to use LZ4 compression algorithm.
 - Modified to use QLatin1String(), QStringLiteral() and QByteArrayLiteral()
   where possible.
 - Fix a bug of incorrect Content-Type returned for files with uppercase
   extension.
 - Fix a bug of parsing form-data values included backslashes.
 - Implemented a function, TMultipartFormData::uploadedFilePath().
 - Implemented a function, TSqlQuery::value(const QString &name).
 - Implemented functions storing data of hash type for Redis.
 - Brushed up THttpRequest class. #234
 - Improved functions of TSqlQuery class. #243
 - Performance improvement.

## 1.23.0
 - Added failsafe of SQL when BEGEN/COMMIT/ROLLBACK failed.
 - Modified to replace QLatin1String with QStringLiteral for view codes.
 - Model optimalizations. #231 #230
 - Minor changes of exception classes. #229
 - Added log messages for query log. #227
 - Added possibility to disable sql transactions in unit test. #226
 - Use delete[] to free aio_buf in aiocb struct. #225
 - Minor changes of testcases for Qt 5.12.
 - Fix a bug of sqlobject for session store.
 - Fix a bug of invoking StaticInitialize() function.
 - Modified not to do keep-alive when action threads equals or more than max
   thread value of config.

## 1.22.0
 - Support for CMake build.
 - Support SMTP connections to older servers. #207
 - Modified log messages.
 - Fix bugs of tspawn.pro file.

## 1.21.0
 - Added the option of linking the shared mongoc library.
 - Fix a bug of line endings of HTTP headers on Windows.
 - Fix a bug of reloading app libraries.
 - Fix to return 404 when the action is not found.

## 1.20.0
 - Implemented rawBody() function in THttpRequest class.
 - Added some functions for getting query string.
 - Added sql statements to PostOpenStatements in database.ini.
 - Fix compilation error on Ubuntu 17.10 and macOS.
 - Fix compilation error on Qt 5.10.

## 1.19.0
 - Added TSqlDriverExtension class for upsert statement.
 - Added QString arguments to sort columns function.
 - Added linkTo() functions that has various arguments.
 - Added itemCountOfCurrentPage() into TPaginator class.
 - Added routes.cfg settings by which redirecting to URL path.
 - Modified to send a message of empty body when the status code is 304.
 - Fix a bug of Windows service stop-request.
 - Fix a bug of routing when any controller is not found.
 - Fix a bug that urlq() function doesn't keep arguments.
 - Fix a bug of printing help messages.
 - Fix compilation error on FreeBSD.

## 1.18.0
 - Implemented a session store for MongoDB.
 - Fix a bug of file session store.
 - Fix a bug of generator of controllers.
 - Fix a bug of parsing arguments when running as Windows service.
 - Fix leak of db connections.

## 1.17.0
 - Fix a bug of comparisn logic of If-Modified-Since header.
 - Fix a bug of URL path traversal.
 - Modified to add view files into the project when tmake was executed.
 - Added logics to invoke a controller constructor by QMetaObject::newInstance()
   function.
 - Added logic of routing to a static file.
 - Added a class to process in background, TBackgroundProcess.

## 1.16.0
 - Added a config for listening IP address.
 - Added a config for executing SQL statements on post-open.
 - Added a function for multi-fields 'order by', tfGetModelListByCriteria().
 - Fix a bug of selecting aarge number of rows in sqlite
 - Modified the generator to generate better code.
 - Added pages for GigHub Pages, English and Japanese.

## 1.15.0
 - Added setValidationError(2) function to TFormValidator class
 - Added debug functions like 'tDebug() << "foo" << ..'.
 - Added config-initializer functions to TWebApplication class.
 - Added C++11 for-loop for TSqlORMapper class.
 - Modified functions of TFormValidator class.
 - Modified to handle quoted multipart/form-data boundary value.
 - Fix a bug of JSON parser of THttpRequest class.
 - Fix a bug of deadlock of thread on win64.
 - Fixed risky fallback for Content-Type in POST handling.
 - Added C++11 for-loop for TSqlORMapper class.
 - Added a fixed width setting for %O parameter for access log.

## 1.14.0
 - Modified to use QThreadStorage class instead of thread_local.
 - Modified the scaffold generater to generate better codes.
 - Modified url() and urla() functions so as to generate the url, searching
   the routing file.
 - Added '#partial' tag in ERB.
 - Added hasEntity() function to TMultipartFormData class.
 - Fix a bug of renderPartial() funcion on Windows.
 - Fix a bug of treefrog termination.
 - Fix a bug of session sqlobject store in PostgreSQL.
 - Fix crash on missing treefrog -k signal argument.
 - Deleted obsolete functions.
 - Performance improvement.

## 1.13.0
 - Implements hazard pointer, lock-free algorithm.
 - Fix a compilation error in MSVC2015.
 - Added TAtomic class, TAtomicPtr class.
 - Performance tuning.
 - Required C++11 compiler.

## 1.12.0
 - Added helper generator to tspawn command.
 - Added inlineImageTag() function.
 - Added functions to Redis driver class.
 - Added modules for server-side JavaScript processing. [Experimental]
 - Fix a bug of mongo driver.

## 1.11.0
 - Updated the mongo C driver to version 1.3.3.
 - Added a optionTags() function.
 - Added variableNameToFieldName() function.
 - Added method chaining to TMongoODMapper class and TSqlORMapper class.
 - Added setSortOrder(const QString &column).
 - Modified the strong trim mode, '2', to delete white spaces and trim views
   strongly.
 - Modified the trim mode for ERB parser, use '.trim_mode' file in
   subdirectories for view.
 - Modified the tmake command to use QLatin1Sring() in preference to tr().
 - Fix a bug of tmake command when c++ code is empty in ERB.
 - Fix a bug of setting cookie for session.

## 1.10.0
 - Implemented a class which represents SQL join clause for the O/R mapper.
 - Implemented LD_PRELOAD mechanism to the treefrog command, Linux only.
 - Added RDB client dlls for 64bit Windows.
 - Fix a bug of TMongoODMapper class.
 - Fix a bug of SQL for SELECT-COUNT query.
 - Fix a bug of getting running application's file path for root user.
 - Support for OS X El capitan.

## 1.9.2
 - Implemented Redis driver.
 - Implemented auto reloading system for application.
 - Fix a bug of receiveing way of TEpollSocket.
 - Fix a bug of timing of calling deleteLater().
 - Fix a bug of segmentation fault on OS X.

## 1.9.1
 - Added 'status' subcommand for treefrog.
 - Added imageLinkTo() method to TViewHelper class.
 - Implemented HTTP send function from WebSocket module.
 - Fix a bug of outputing a newline code by tmake command.
 - Fix compilation error on Qt5.5
 - Enhanced C++11 support.

## 1.9.0
 - Implemented keep-alive modules for WebSocket.
 - Implemented publish/subscribe functions for WebSocket.
 - Changed API of endpoint class.
 - Changed default random generator to std::mt19937.
 - Changed system bus module so as to use local socket class.
 - Added TSql::IsEmpty operator for TCriteria class.
 - Added NOT operator for TCriteria class.
 - Fix a bug of parsing WebSocket frame.
 - Fix a bug of sending logic for epoll.
 - Fix a bug of segmentation error.
 - Fix a bug of throwing runtime exception of treefrog.
 - Fix a buf of windows installer.
 - Performance improvement of multiplexing server.

## 1.8.0
 - Support WebSocket protocol.
 - C++11 support enabled as default.
 - Performance improvement in hybrid MPM.
 - Fix a bug of 'abort' subcommand of treefrog on Windows.
 - Fix a bug of routing URL.
 - Modified to use std::atomic instead of QAtomic.
 - Modified to catch SIGINT in case of debug mode.
 - Modified so as not to use Tf::currentDateTimeSec() function.
 - Modified to collect settings infomation to TAppSettings class.
 - Added hint macros, QLIKELY/QUNLIKELY.
 - Unsupported prefork MPM.
 - Deleted obsolete functions.

## 1.7.9
 - Support for Raspberry Pi, ARM architecture.
 - Fix a bug of settings of default project file on Qt4.

## 1.7.8
 - Added a debug mode option for tadpole command.
 - Fix a bug of TSqlObject for PostgresSQL.
 - Fix a bug that QPSQLResult doesn't call QSqlField::setAutoValue().
 - Changed for work of tfmanager in Windows service mode.
 - Changed a default project file, app.pro.
 - Changed a signal-handler message.
 - Changed the default project file to build views.
 - Support for Visual Studio 2013. [Experimental]

## 1.7.7
 - Routing enhancements, added ':param' parameter for routes.cfg.
 - Added a directive, EnableHttpMethodOverride, of application.ini.
 - Added code of checking parameters to addRouteFromString().
 - Added a test case, 'urlrouter'.
 - Added checkBoxTag() function.
 - Fix a bug of 'abort' option.

## 1.7.6
 - Modified to specify the max connections of database pool.
 - Changed identityKeyOfLoginUser() to public function.
 - Added ApplicationController::staticRelease() function.
 - Added StandardException class.
 - Implemented TStaticReleaseThread class.
 - Implemented command-line interface.
 - Added setAllowUnauthenticatedUser(1) and setDenyUnauthenticatedUser(1)
    functions into TAccessValidator class.
 - Added the permissions parameter to renameUploadedFile().
 - Modified save() function to call create() instead of mdata()->create().
 - Fix a bug of files not generated with tspawn.
 - Fix a bug of ERB generation for mailer directory.
 - Implemented Sendmail client. [Experimental]

## 1.7.5
 - Performance improvement of multiplexing server.
 - Modified to start multiple application servers under 'thread' MPM.
 - Added checkBoxTag(4) and radioButtonTag(4) functions into TViewHelper
    class.
 - Added selectTag() and optionTags() functions into TViewHelper class.
 - Fix a bug of inserting a record that has a primary key.
 - Fix bugs of 'restart' option of treefrog command.
 - Changed parameter names in ini file.
 - Modified to flush access logging.
 - Modified to update updated_at field of SQL object with the current
    date/time in updateAll() function.
 - Added new classes, TScheduler and TApplicationScheduler. [Experimental]
 - Added max number of buffering data under async I/O.
 - Modified to use async I/O file writer for logging.
 - Fix a bug of TID printing.
 - Fix a bug of specifing month string of the Date header.
 - Fix a bug of 'abort' option of treefrog command.
 - Fix build error of ORM class.
 - Fix a bug of commiting a transacton under HTTP-pipeline.
 - Fix a bug of access validation of non-login user.
 - Fix a bug of generating source codes of MongoDB.

## 1.7.4
 - Fix a bug of connection error under high load.

## 1.7.3
 - Implemented asynchronous log outputting.
 - Support HTTP-pipeline mechanism (HTTP 1.1).
 - Updated a parser of HTTP header.
 - Modified output logics of query log.
 - Added HMAC_SHA256, HMAC_SHA384 and HMAC_SHA512 algorithm.
 - Fix a bug of specifing thread ID on logging.
 - Modified to use accept4() in multiplexing app server.
 - Implemented getUTCTimeString() and Tf::currentDateTimeSec() functions.

## 1.7.2
 - Fix a bug of plugin loading in Qt5.
 - Fix a bug of specifying Mongo operators.
 - Fix a bug of access validation.
 - Fix a bug of request receiving of multipart/form-data.
 - Fix a bug of configure execution in a spaced path.
 - Modified program logic for scaffolding.
 - Added implement receiving JSON data of post method.
 - Updated to MongoDB C Driver v0.8.1.

## 1.7.1
 - Fix a bug of linkToIf() and linkToUnless().
 - Fix a bug of user-model generation.
 - Added findCount() into classes generated by tspawn command.
 - Added urlq() function into TActionHelper class.
 - Update the default encoding of JSON response in HTTP, 'charset=utf-8'.
 - Imports new files, tjsonutil.cpp and tjsonutil.h and add utility functions
    for JSON.
 - Deleted TF_BUILD_MONGODB macros
 - Added '--show-collections' option to tspawn command for MongoDB.
 - Updated oidInc() function in tbson.cpp
 - Updated the configure script.

## 1.7.0
 - Added a multiplexing socket receiver using epoll system call (Linux only)
 - Update Mongo C driver to v0.8.
 - Modified to use atomic functions instead of mutex locking.
 - Improving Performance of multiplexing receiver.
 - Added 'hybrid' as multi-processing module (MPM)
 - Added the THttpSendBuffer class for send buffering.
 - Fix compile error in Qt v5.1.
 - Added a atomic queue module, TAtomicQueue class.
 - Added a object-document mapper module for MongoDB, MongoODMapper class.
 - Supported MongoDB object as model objects.
 - Added a criteria converter for MongoDB, TCriteriaMongoConverter class.
 - Added new functions to TMongoQuery class.
 - Changed a patameter type of updateAll() to QMap<int,QVariant> class.
 - Fix a bug of execution of SQL query in a constuctor.
 - Added the option of Mongo object creation to 'tspawn' generator.

## 1.6.1
 - Fix a bug of generating user models.
 - Fix a bug of case-sensitivity of field names in DB tables.
 - Fix compile error when the 'gui_mod' flag is on.
 - Fix a bug of the 'TSql::In' statement used.
 - Performance improvement.
 - Added a class for KVS exception, KvsException.
 - Added a function for 'POP brefore SMTP' auth.
 - Added methods for TSqlORMapper, findFirstBy(), findCount(), findBy() and
    findIn().
 - Mac: Fix a bug of including path of appbase.pri.
 - MongoDB: Added functions for MongoDB access, findById(), removeById()
    and updateById().
 - MongoDB: Fix a bug of updateMulti() method in TMongoQuery class.
 - MongoDB: Added numDocsAffected() function into the TMongoQuery class.
 - MongoDB: Modified to generate a ObjectId on client.

## 1.6
 - Performance improvement.
 - JSON supported for AJAX, added sendJson() method into TActionController.
 - XML supported for AJAX, added new method, renderXml().
 - Changed the common data format from QVariantHash to QVariantMap for JSON
    conversion.
 - Bugfix of creating the 'ORDER BY' clause.
 - Updated the configure script.
 - Fixed compile error on Qt-4.6
 - MongoDB supported for easy access.  [Experimental]

## 1.3
 - Qt version 5 supported.
 - Added a version string of Windows8.
 - Added a local socket mechanism for killing tfmanager.
 - Implemented '-l' option for treefrog command.
 - Fix error in writing.

## 1.2
 - TreeFrog app-server for Windows service supported.
 - Modified the logic of parsing HTML text.
 - Added parentExists() function of tmake command.
 - Modified parseWord() function.
 - Modified help message of tfmanager command.
 - Modified it not to use gettimeofday() function.
 - Changed a class name, TAccessAuthenticator -> TAccessValidator.
 - Added the method TActionController::availableControllers().
 - Implements access auth of users into TActionController.
 - Added removeRawHeader() into TInternetMessageHeader class.

## 1.1
 - Bugfix of parsing a boundary of HTTP request.
 - Implemented releaseDatabases() function.
 - Implemented tehex2() macro.
 - Implemented htmlEscape(int n, Tf::EscapeFlag f) function.
 - Implemented '%|%' tag for echo a default value on ERB system
 - Implemented setContentType() function into TActionController class.
 - Added a escape-flag parameter to the htmlEscape() function.
 - Bugfix of calling staticInitialize().
 - Multi-database access supported.
 - Modified the URL validation to be strict.
 - Added various setRule() functions.
 - Implemented a query parameter of src of image-tag.
 - Modified ApplicationController template class.
 - Added TStaticInitializer class for prefork module.
 - Implemented logic of calling staticInitialize() of ApplicationController.
 - Added typeName() function to TDispatcher.
 - Added a restart command into tfmanager.
 - Added resetSignalNumber() function into TWebApplication.
 - Added a logics of checking idle time of socket recieving.
 - Modifiied that tfmanager opens a socket in case of Prefork only.
 - Added a OpenFlag parameter to nativeListen() function.
 - Modified the way of call nativeListen() function.
 - Move nativeListen() function into TApplicationServer class, and
    added --ctrlc-enable option to tadpole command.
 - Changed enum valus, UserDefined -> Pattern, and etc.
 - Renamed class name, THashValidator -> TFormValidator.
 - Added to output SQL query log.
 - Modified a function name to be called.
 - Update a parameter, QHash<QString,QString> -> QVariantHash.
 - Added setValidationError() function for custom validation.
 - Writes stderr output of tfserver to a debug file.
 - Modified parameters of imageLinkTo() function.
 - Imports new setting file, development.ini.
 - Modified thattmake and tspawn commands refer to the
    'TemplateSystem' setting of the development.ini file.
 - Added a defaultValue parameter to queryItemValue() and
    formItemValue() method each.
 - Added a parameter of a query string to the url() fucntion.
 - Added renderErrorResponse() method into the TActionController class.
 - Modified access log output.
 - Implemented UNIX domain socket.
 - Modified to check the socket's timeout.
 - The default prefix of Otama marking was changed, '#' -> '@'.
 - Added to set a default value into TAccessAuthenticator::clear() function.
 - Added a method, redirectToPage().
 - Added to install defaults/403.html.
 - Added new class, TAccessAuthenticator.
 - Added a method, currentController(), to TActionContext class.
 - Modified to update only DB-fields whose values were changed.
 - Fix a bug of generating a user-model.
 - Added logic of initializing member variables of model class to
    the model-generater.
 - Added to create a Makefile by qmake command.
 - Added validate function.
 - Fix a bug of TSqlORMapperIterator and TSqlQueryORMapperIterator.
 - Added new files, tsqlqueryormapperiterator.h and
    TSqlQueryORMapperIterator.
 - Modified the posision of 'new entry' on entry.erb and entry.html.
 - Modified generator logics; controller, model and view.
 - Added url and urla() functons with a QVariant parameter.
 - Added an auto-update logic of 'modified_at'.
 - Changed a form type, QHash<QString,QString> -> QVariantHash.
 - Added typedef QHash<QString, QString>.
 - Modified the creating logic of URL of urala() function.
 - Modified the method name, setActionView() -> setSubActionView().
 - Modified static files for error, 500.html, 404.html and 413.html
 - Fix a bug of order of 'ORDER BY' phrase.
 - Modified -d option's message.
 - Modified a method name, allExportVariants -> allVariants.
