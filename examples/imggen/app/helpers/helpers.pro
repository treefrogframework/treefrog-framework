TARGET = helper
TEMPLATE = lib
CONFIG += shared
QT  -= gui
QT  += 
DEFINES += TF_DLL
DESTDIR = ../../lib
DEPENDPATH +=

include(../appbase.pri)

HEADERS += applicationhelper.h
SOURCES += applicationhelper.cpp
