TARGET = helper
TEMPLATE = lib
CONFIG += shared debug
QT  -= gui
QT  += 
DEFINES += TF_DLL
DESTDIR = ../../lib
DEPENDPATH +=

include(../appbase.pri)

HEADERS += applicationhelper.h
SOURCES += applicationhelper.cpp
HEADERS += hellohelper.h
SOURCES += hellohelper.cpp
