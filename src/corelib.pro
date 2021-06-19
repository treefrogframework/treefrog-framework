TARGET   = treefrog
TEMPLATE = lib
CONFIG  += shared console
CONFIG  -= lib_bundle
QT      += sql network xml qml
lessThan(QT_MAJOR_VERSION, 6) {
  CONFIG += c++14
  windows:QMAKE_CXXFLAGS += /std:c++14
} else {
  CONFIG += c++17
  QT += core5compat
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17
}

DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES += TF_MAKEDLL
DEFINES += QT_DEPRECATED_WARNINGS
INCLUDEPATH += ../include ../3rdparty/lz4/lib
DEPENDPATH  += ../include
MOC_DIR = .obj/
OBJECTS_DIR = .obj/
windows:CONFIG(debug, debug|release) {
  TARGET = $$join(TARGET,,,d)
}

include(../tfbase.pri)
include(../include/headers.pri)
VERSION = $$TF_VERSION

isEmpty(target.path) {
  windows {
    target.path = C:/TreeFrog/$${VERSION}/bin
  } else:macx {
    target.path = /usr/local/lib
  } else:unix {
    target.path = /usr/lib
  }
}
INSTALLS += target

windows {
  win32-msvc* {
    LIBS += ../3rdparty/lz4/build/liblz4_static.lib
  } else {
    LIBS += ../3rdparty/lz4/lib/release/liblz4.a
  }

  header.files = $$HEADER_FILES $$HEADER_CLASSES
  header.files += $$MONGODB_FILES $$MONGODB_CLASSES
  lessThan(QT_MAJOR_VERSION, 6) {
    win32-msvc* {
      QMAKE_CXXFLAGS += /source-charset:utf-8 /wd 4819 /wd 4661
    }
  } else {
    win32-msvc* {
      QMAKE_CXXFLAGS += /wd 4819 /wd 4661
    }
  }

  isEmpty(header.path) {
    header.path = C:/TreeFrog/$${VERSION}/include
  }
  script.files = ../tfenv.bat
  script.path = $$target.path
  test.files = $$TEST_FILES $$TEST_CLASSES
  test.path = $$header.path/TfTest
  INSTALLS += header script test
} else:unix {
  LIBS += ../3rdparty/lz4/lib/liblz4.a
  macx:QMAKE_SONAME_PREFIX=@rpath

  header.files = $$HEADER_FILES $$HEADER_CLASSES
  header.files += $$MONGODB_FILES $$MONGODB_CLASSES

  test.files = $$TEST_FILES $$TEST_CLASSES
  test.path = $$header.path/TfTest
  INSTALLS += header test
}

!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

isEmpty( use_gui ) {
  QT    -= gui widgets
} else {
  QT    += gui widgets
  DEFINES += TF_USE_GUI_MODULE
}

!windows {
  CONFIG += precompile_header
  PRECOMPILED_HEADER = precompile.h
}

HEADERS += twebapplication.h
SOURCES += twebapplication.cpp
HEADERS += tapplicationserverbase.h
SOURCES += tapplicationserverbase.cpp
HEADERS += tthreadapplicationserver.h
SOURCES += tthreadapplicationserver.cpp
HEADERS += tactioncontext.h
SOURCES += tactioncontext.cpp
HEADERS += tdatabasecontext.h
SOURCES += tdatabasecontext.cpp
HEADERS += tactionthread.h
SOURCES += tactionthread.cpp
#HEADERS += tactionforkprocess.h
#SOURCES += tactionforkprocess.cpp
HEADERS += thttpsocket.h
SOURCES += thttpsocket.cpp
HEADERS += thttpclient.h
SOURCES += thttpclient.cpp
#HEADERS += thttp2socket.h
#SOURCES += thttp2socket.cpp
#HEADERS += thttpbuffer.h
#SOURCES += thttpbuffer.cpp
HEADERS += tsendbuffer.h
SOURCES += tsendbuffer.cpp
HEADERS += tabstractcontroller.h
SOURCES += tabstractcontroller.cpp
HEADERS += tactioncontroller.h
SOURCES += tactioncontroller.cpp
HEADERS += directcontroller.h
SOURCES += directcontroller.cpp
HEADERS += tactionview.h
SOURCES += tactionview.cpp
HEADERS += tactionmailer.h
SOURCES += tactionmailer.cpp
HEADERS += tsqldatabase.h
SOURCES += tsqldatabase.cpp
HEADERS += tsqldatabasepool.h
SOURCES += tsqldatabasepool.cpp
HEADERS += tsqlobject.h
SOURCES += tsqlobject.cpp
HEADERS += tsqlormapperiterator.h
SOURCES += tsqlormapperiterator.cpp
HEADERS += tsqlquery.h
SOURCES += tsqlquery.cpp
HEADERS += tsqlqueryormapper.h
SOURCES += tsqlqueryormapper.cpp
HEADERS += tsqlqueryormapperiterator.h
SOURCES += tsqlqueryormapperiterator.cpp
HEADERS += tsqltransaction.h
SOURCES += tsqltransaction.cpp
HEADERS += tsqldriverextension.h
SOURCES += tsqldriverextension.cpp
HEADERS += tsqldriverextensionfactory.h
SOURCES += tsqldriverextensionfactory.cpp
HEADERS += tcriteria.h
SOURCES += tcriteria.cpp
HEADERS += tcriteriaconverter.h
SOURCES += tcriteriaconverter.cpp
HEADERS += thttprequest.h
SOURCES += thttprequest.cpp
HEADERS += thttpresponse.h
SOURCES += thttpresponse.cpp
HEADERS += tmultipartformdata.h
SOURCES += tmultipartformdata.cpp
HEADERS += tcontentheader.h
SOURCES += tcontentheader.cpp
HEADERS += thttputility.h
SOURCES += thttputility.cpp
HEADERS += thtmlattribute.h
SOURCES += thtmlattribute.cpp
HEADERS += ttextview.h
SOURCES += ttextview.cpp
HEADERS += tdirectview.h
SOURCES += tdirectview.cpp
HEADERS += tactionhelper.h
SOURCES += tactionhelper.cpp
HEADERS += tviewhelper.h
SOURCES += tviewhelper.cpp
HEADERS += tprototypeajaxhelper.h
SOURCES += tprototypeajaxhelper.cpp
HEADERS += toption.h
SOURCES += toption.cpp
HEADERS += ttemporaryfile.h
SOURCES += ttemporaryfile.cpp
HEADERS += tcookie.h
SOURCES += tcookie.cpp
HEADERS += tcookiejar.h
SOURCES += tcookiejar.cpp
HEADERS += tsession.h
SOURCES += tsession.cpp
HEADERS += tsessionmanager.h
SOURCES += tsessionmanager.cpp
HEADERS += tsessionstore.h
SOURCES += tsessionstore.cpp
HEADERS += tsessionstorefactory.h
SOURCES += tsessionstorefactory.cpp
HEADERS += tsessionsqlobjectstore.h
SOURCES += tsessionsqlobjectstore.cpp
HEADERS += tsessionmongostore.h
SOURCES += tsessionmongostore.cpp
HEADERS += tsessioncookiestore.h
SOURCES += tsessioncookiestore.cpp
HEADERS += tsessionfilestore.h
SOURCES += tsessionfilestore.cpp
HEADERS += tsessionredisstore.h
SOURCES += tsessionredisstore.cpp
HEADERS += thtmlparser.h
SOURCES += thtmlparser.cpp
HEADERS += tabstractmodel.h
SOURCES += tabstractmodel.cpp
HEADERS += tmodelutil.h
#SOURCES += tmodelutil.cpp
HEADERS += tmodelobject.h
SOURCES += tmodelobject.cpp
HEADERS += tsystemglobal.h
SOURCES += tsystemglobal.cpp
HEADERS += tglobal.h
SOURCES += tglobal.cpp
HEADERS += taccesslog.h
SOURCES += taccesslog.cpp
HEADERS += taccesslogstream.h
SOURCES += taccesslogstream.cpp
HEADERS += tlog.h
SOURCES += tlog.cpp
HEADERS += tlogger.h
SOURCES += tlogger.cpp
HEADERS += tloggerfactory.h
SOURCES += tloggerfactory.cpp
HEADERS += tfilelogger.h
SOURCES += tfilelogger.cpp
HEADERS += tabstractlogstream.h
SOURCES += tabstractlogstream.cpp
HEADERS += tsharedmemorylogstream.h
SOURCES += tsharedmemorylogstream.cpp
HEADERS += tbasiclogstream.h
SOURCES += tbasiclogstream.cpp
#HEADERS += tmailerfactory.h
#SOURCES += tmailerfactory.cpp
HEADERS += tmailmessage.h
SOURCES += tmailmessage.cpp
HEADERS += tsmtpmailer.h
SOURCES += tsmtpmailer.cpp
HEADERS += tpopmailer.h
SOURCES += tpopmailer.cpp
HEADERS += tsendmailmailer.h
SOURCES += tsendmailmailer.cpp
HEADERS += tcryptmac.h
SOURCES += tcryptmac.cpp
HEADERS += tinternetmessageheader.h
SOURCES += tinternetmessageheader.cpp
HEADERS += thttpheader.h
SOURCES += thttpheader.cpp
HEADERS += turlroute.h
SOURCES += turlroute.cpp
HEADERS += tabstractuser.h
SOURCES += tabstractuser.cpp
HEADERS += tformvalidator.h
SOURCES += tformvalidator.cpp
HEADERS += taccessvalidator.h
SOURCES += taccessvalidator.cpp
HEADERS += tpaginator.h
SOURCES += tpaginator.cpp
HEADERS += tkvsdatabase.h
SOURCES += tkvsdatabase.cpp
HEADERS += tkvsdatabasepool.h
SOURCES += tkvsdatabasepool.cpp
HEADERS += tkvsdriver.h
SOURCES += tkvsdriver.cpp
HEADERS += tredisdriver.h
SOURCES += tredisdriver.cpp
HEADERS += tredis.h
SOURCES += tredis.cpp
HEADERS += tfileaiologger.h
SOURCES += tfileaiologger.cpp
HEADERS += tfileaiowriter.h
SOURCES += tfileaiowriter.cpp
HEADERS += tjobscheduler.h
SOURCES += tjobscheduler.cpp
HEADERS += tappsettings.h
SOURCES += tappsettings.cpp
HEADERS += tabstractwebsocket.h
SOURCES += tabstractwebsocket.cpp
HEADERS += twebsocket.h
SOURCES += twebsocket.cpp
HEADERS += twebsocketendpoint.h
SOURCES += twebsocketendpoint.cpp
HEADERS += twebsocketframe.h
SOURCES += twebsocketframe.cpp
HEADERS += twebsocketworker.h
SOURCES += twebsocketworker.cpp
HEADERS += twebsocketsession.h
SOURCES += twebsocketsession.cpp
HEADERS += tpublisher.h
SOURCES += tpublisher.cpp
HEADERS += tsystembus.h
SOURCES += tsystembus.cpp
HEADERS += tprocessinfo.h
SOURCES += tprocessinfo.cpp
HEADERS += tbasictimer.h
SOURCES += tbasictimer.cpp
HEADERS += tatomicptr.h
SOURCES += tatomicptr.cpp
HEADERS += thazardptr.h
SOURCES += thazardptr.cpp
HEADERS += thazardobject.h
SOURCES += thazardobject.cpp
HEADERS += thazardptrmanager.h
SOURCES += thazardptrmanager.cpp
#HEADERS += tsinglylist.h
#SOURCES += tsinglylist.cpp
HEADERS += tatomic.h
SOURCES += tatomic.cpp
HEADERS += tstack.h
SOURCES += tstack.cpp
HEADERS += tqueue.h
SOURCES += tqueue.cpp
HEADERS += tdatabasecontextthread.h
SOURCES += tdatabasecontextthread.cpp
HEADERS += tdatabasecontextmainthread.h
SOURCES += tdatabasecontextmainthread.cpp
HEADERS += tbackgroundprocess.h
SOURCES += tbackgroundprocess.cpp
HEADERS += tbackgroundprocesshandler.h
SOURCES += tbackgroundprocesshandler.cpp
HEADERS += tdebug.h
SOURCES += tdebug.cpp
HEADERS += tjsonutil.h
SOURCES += tjsonutil.cpp
HEADERS += tjsloader.h
SOURCES += tjsloader.cpp
HEADERS += tjsmodule.h
SOURCES += tjsmodule.cpp
HEADERS += tjsinstance.h
SOURCES += tjsinstance.cpp
HEADERS += treactcomponent.h
SOURCES += treactcomponent.cpp
HEADERS += tcache.h
SOURCES += tcache.cpp
HEADERS += tcachefactory.h
SOURCES += tcachefactory.cpp
HEADERS += tcachestore.h
SOURCES += tcachestore.cpp
HEADERS += tcachesqlitestore.h
SOURCES += tcachesqlitestore.cpp
HEADERS += tcachemongostore.h
SOURCES += tcachemongostore.cpp
HEADERS += tcacheredisstore.h
SOURCES += tcacheredisstore.cpp
SOURCES += tactioncontroller_qt5.cpp
HEADERS += toauth2client.h
SOURCES += toauth2client.cpp

HEADERS += \
           tfnamespace.h \
           tfcore.h \
           tfexception.h \
           tdispatcher.h \
           tloggerplugin.h \
           tsessionobject.h \
           tsessionmongoobject.h \
           tsessionstoreplugin.h \
           tjavascriptobject.h \
           tsqlormapper.h \
           tsqljoin.h \
           thttprequestheader.h \
           thttpresponseheader.h \
           tcommandlineinterface.h

windows {
  HEADERS += tfcore_win.h
  SOURCES += twebapplication_win.cpp
  SOURCES += tapplicationserverbase_win.cpp
  SOURCES += tfileaiowriter_win.cpp
  SOURCES += tprocessinfo_win.cpp
  SOURCES += tredisdriver_win.cpp
  SOURCES += tthreadapplicationserver_qt5.cpp
}
unix {
  HEADERS += tfcore_unix.h
  SOURCES += twebapplication_unix.cpp
  SOURCES += tapplicationserverbase_unix.cpp
  SOURCES += tfileaiowriter_unix.cpp
  SOURCES += tredisdriver_unix.cpp
}
linux-* {
  HEADERS += tmultiplexingserver.h
  SOURCES += tmultiplexingserver_linux.cpp
  HEADERS += tactionworker.h
  SOURCES += tactionworker.cpp
  HEADERS += tepoll.h
  SOURCES += tepoll.cpp
  HEADERS += tepollsocket.h
  SOURCES += tepollsocket.cpp
  HEADERS += tepollhttpsocket.h
  SOURCES += tepollhttpsocket.cpp
  HEADERS += tepollwebsocket.h
  SOURCES += tepollwebsocket.cpp
  SOURCES += tprocessinfo_linux.cpp
  SOURCES += tthreadapplicationserver_linux.cpp
}
macx {
  SOURCES += tprocessinfo_macx.cpp
  SOURCES += tthreadapplicationserver_qt5.cpp
}
freebsd {
  SOURCES += tprocessinfo_freebsd.cpp
  LIBS += -lutil -lprocstat
}


# Files for MongoDB
windows {
  DEFINES += MONGOC_COMPILATION BSON_COMPILATION
  INCLUDEPATH += ../3rdparty/mongo-driver/src/mongoc ../3rdparty/mongo-driver/src/libbson/src/bson
  win32-msvc* {
    LIBS += ../3rdparty/mongo-driver/Release/mongoc-static-1.0.lib ../3rdparty/mongo-driver/src/libbson/Release/bson-static-1.0.lib
  } else {
    CONFIG(debug, debug|release) {
      LIBS += ../3rdparty/mongo-driver/debug/libmongoc.a
    } else {
      LIBS += ../3rdparty/mongo-driver/release/libmongoc.a
    }
  }
  LIBS += -lws2_32 -lpsapi -lAdvapi32
} else {
  isEmpty( shared_mongoc ) {
    INCLUDEPATH += ../3rdparty/mongo-driver/src/mongoc ../3rdparty/mongo-driver/src/libbson/src/bson
    LIBS += ../3rdparty/mongo-driver/libmongoc-static-1.0.a ../3rdparty/mongo-driver/src/libbson/libbson-static-1.0.a
  } else {
    macx {
      # Homebrew
      INCLUDEPATH += /usr/local/include/libmongoc-1.0 /usr/local/include/libbson-1.0
      LIBS += -L/usr/local/lib
    } else {
      INCLUDEPATH += /usr/include/libmongoc-1.0 /usr/include/libbson-1.0
    }
    LIBS += -lmongoc-1.0 -lbson-1.0
  }
}

HEADERS += tmongodriver.h
SOURCES += tmongodriver.cpp
HEADERS += tmongoquery.h
SOURCES += tmongoquery.cpp
HEADERS += tmongocursor.h
SOURCES += tmongocursor.cpp
HEADERS += tbson.h
SOURCES += tbson.cpp
HEADERS += tmongoobject.h
SOURCES += tmongoobject.cpp
HEADERS += tmongoodmapper.h
SOURCES += tmongoodmapper.cpp
HEADERS += tcriteriamongoconverter.h
SOURCES += tcriteriamongoconverter.cpp
