TARGET = controller
TEMPLATE = lib
CONFIG += shared debug
QT -= gui
QT += network sql
DEFINES += TF_DLL
DESTDIR = ../../lib
INCLUDEPATH += ../helpers ../models
DEPENDPATH  += ../helpers ../models
LIBS  += -L../../lib -lhelper -lmodel

include(../appbase.pri)

HEADERS += applicationcontroller.h
SOURCES += applicationcontroller.cpp
HEADERS += hellocontroller.h
SOURCES += hellocontroller.cpp
HEADERS += indexcontroller.h
SOURCES += indexcontroller.cpp
HEADERS += entrycontroller.h
SOURCES += entrycontroller.cpp
