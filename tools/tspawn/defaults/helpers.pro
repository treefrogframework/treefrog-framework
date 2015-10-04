TARGET = helper
TEMPLATE = lib
CONFIG += shared c++11 x86_64
QT  -= gui
QT  += 
DEFINES += TF_DLL
DESTDIR = ../lib
DEPENDPATH +=

include(../appbase.pri)

HEADERS += applicationhelper.h
SOURCES += applicationhelper.cpp
