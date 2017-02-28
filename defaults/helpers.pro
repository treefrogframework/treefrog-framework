TARGET = helper
TEMPLATE = lib
CONFIG += shared c++11
QT += xml
QT -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += qml
DEFINES += TF_DLL
DESTDIR = ../lib
INCLUDEPATH +=
DEPENDPATH  +=
LIBS +=
MOC_DIR = .obj/
OBJECTS_DIR = .obj/

include(../appbase.pri)

HEADERS += applicationhelper.h
SOURCES += applicationhelper.cpp
