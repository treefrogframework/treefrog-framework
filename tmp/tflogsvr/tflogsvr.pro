TARGET = tflogsvr
TEMPLATE = app
VERSION = 1.0.0
CONFIG += console
CONFIG -= app_bundle
QT     += network
QT     -= gui
DEFINES += TF_DLL

include(../tfbase.pri)

isEmpty( target.path ) {
  win32 {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}
INSTALLS += target

!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp
HEADERS += logserver.h
SOURCES += logserver.cpp
