TARGET   = treefrog
TEMPLATE = lib
CONFIG  += shared console c++11
QT      += sql network xml
DEFINES += TF_MAKEDLL
INCLUDEPATH += ../include
DEPENDPATH  += ../include
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
    target.path = /Library/Frameworks
  } else:unix {
    target.path = /usr/lib
  }
}
INSTALLS += target

windows {
  LIBS += -lws2_32 -lpsapi
  header.files = $$HEADER_FILES $$HEADER_CLASSES
  header.files += $$MONGODB_FILES $$MONGODB_CLASSES

  isEmpty(header.path) {
    header.path = C:/TreeFrog/$${VERSION}/include
  }
  script.files = ../tfenv.bat
  script.path = $$target.path
  test.files = $$TEST_FILES $$TEST_CLASSES
  test.path = $$header.path/TfTest
  INSTALLS += header script test
} else:macx {
  CONFIG += lib_bundle
  FRAMEWORK_HEADERS.version = Versions
  FRAMEWORK_HEADERS.files = $$HEADER_FILES $$HEADER_CLASSES
  FRAMEWORK_HEADERS.files += $$MONGODB_FILES $$MONGODB_CLASSES

  FRAMEWORK_HEADERS.path = Headers
  FRAMEWORK_TEST.version = Versions
  FRAMEWORK_TEST.files = $$TEST_FILES $$TEST_CLASSES
  FRAMEWORK_TEST.path = Headers/TfTest
  QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS FRAMEWORK_TEST
} else:unix {
  header.files = $$HEADER_FILES $$HEADER_CLASSES
  header.files += $$MONGODB_FILES $$MONGODB_CLASSES

  isEmpty(header.path) {
    header.path = /usr/include/treefrog
  }
  test.files = $$TEST_FILES $$TEST_CLASSES
  test.path = $$header.path/TfTest
  INSTALLS += header test

  # c++11
  lessThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=c++0x
  }
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

HEADERS += twebapplication.h
SOURCES += twebapplication.cpp
HEADERS += tapplicationserverbase.h
SOURCES += tapplicationserverbase.cpp
HEADERS += tthreadapplicationserver.h
SOURCES += tthreadapplicationserver.cpp
#HEADERS += tpreforkapplicationserver.h
#SOURCES += tpreforkapplicationserver.cpp
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
HEADERS += tcookiejar.h
SOURCES += tcookiejar.cpp
HEADERS += tsession.h
SOURCES += tsession.cpp
HEADERS += tsessionmanager.h
SOURCES += tsessionmanager.cpp
HEADERS += tsessionstorefactory.h
SOURCES += tsessionstorefactory.cpp
HEADERS += tsessionsqlobjectstore.h
SOURCES += tsessionsqlobjectstore.cpp
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
#HEADERS += tatomicset.h
#SOURCES += tatomicset.cpp
HEADERS += tatomicqueue.h
SOURCES += tatomicqueue.cpp
HEADERS += tfileaiologger.h
SOURCES += tfileaiologger.cpp
HEADERS += tfileaiowriter.h
SOURCES += tfileaiowriter.cpp
HEADERS += tscheduler.h
SOURCES += tscheduler.cpp
HEADERS += tapplicationscheduler.h
SOURCES += tapplicationscheduler.cpp
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

HEADERS += \
           tfnamespace.h \
           tfcore.h \
           tfexception.h \
           tcookie.h \
           tdispatcher.h \
           tloggerplugin.h \
           tsessionobject.h \
           tsessionstore.h \
           tsessionstoreplugin.h \
           tjavascriptobject.h \
           tsqlormapper.h \
           tsqljoin.h \
           thttprequestheader.h \
           thttpresponseheader.h \
           tcommandlineinterface.h

windows {
  SOURCES += twebapplication_win.cpp
  SOURCES += tapplicationserverbase_win.cpp
  SOURCES += tfileaiowriter_win.cpp
  SOURCES += tprocessinfo_win.cpp
}
unix {
  HEADERS += tfcore_unix.h
  SOURCES += twebapplication_unix.cpp
  SOURCES += tapplicationserverbase_unix.cpp
  SOURCES += tfileaiowriter_unix.cpp
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
}
macx {
  SOURCES += tprocessinfo_macx.cpp
}

# Qt5
greaterThan(QT_MAJOR_VERSION, 4) {
  HEADERS += tjsonutil.h
  SOURCES += tjsonutil.cpp

  SOURCES += tactioncontroller_qt5.cpp
}


# Files for MongoDB
INCLUDEPATH += ../3rdparty/mongo-c-driver/src
windows {
  DEFINES += MONGO_STATIC_BUILD

  win32-msvc* {
    CONFIG(debug, debug|release) {
      LIBS += ..\3rdparty\mongo-c-driver\debug\mongoc.lib
    } else {
      LIBS += ..\3rdparty\mongo-c-driver\release\mongoc.lib
    }
  } else {
    CONFIG(debug, debug|release) {
      LIBS += ../3rdparty/mongo-c-driver/debug/libmongoc.a -lws2_32
    } else {
      LIBS += ../3rdparty/mongo-c-driver/release/libmongoc.a -lws2_32
    }
  }
} else {
  LIBS += ../3rdparty/mongo-c-driver/libmongoc.a
}
DEFINES += MONGO_HAVE_STDINT

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
