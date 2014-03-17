include(../test.pri)

TARGET = buildtest
QT += sql network

SOURCES += main.cpp
HEADERS += blog.h
SOURCES += blog.cpp
HEADERS += blogobject.h
HEADERS += foo.h
SOURCES += foo.cpp
HEADERS += fooobject.h

