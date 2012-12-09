TARGET = model
TEMPLATE = lib
CONFIG += shared x86_64
QT  -= gui
QT  += sql network
DEFINES += TF_DLL
DESTDIR = ../lib
INCLUDEPATH += ../helpers sqlobjects
DEPENDPATH  += ../helpers sqlobjects
LIBS += -L../lib -lhelper

include(../appbase.pri)

HEADERS += entrynameobject.h
HEADERS += entryname.h
SOURCES += entryname.cpp
HEADERS += entryobject.h
HEADERS += entry.h
SOURCES += entry.cpp
#HEADERS += entry2.h
#SOURCES += entry2.cpp

