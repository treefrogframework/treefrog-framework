TARGET = model
TEMPLATE = lib
CONFIG += shared
QT += sql
QT -= gui
DEFINES += TF_DLL
DESTDIR = ../lib
INCLUDEPATH += ../helpers sqlobjects mongoobjects
DEPENDPATH  += ../helpers sqlobjects mongoobjects
LIBS += -L../lib -lhelper

include(../appbase.pri)

