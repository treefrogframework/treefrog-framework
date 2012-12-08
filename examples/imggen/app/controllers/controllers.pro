TARGET = controller
TEMPLATE = lib
CONFIG += shared
QT += network sql
#QT -= gui
DEFINES += TF_DLL
DESTDIR = ../../lib
INCLUDEPATH += ../helpers ../models
DEPENDPATH  += ../helpers ../models
LIBS  += -L../../lib -lhelper
include(../appbase.pri)

HEADERS += applicationcontroller.h
SOURCES += applicationcontroller.cpp
HEADERS += imagecreatorcontroller.h
SOURCES += imagecreatorcontroller.cpp
