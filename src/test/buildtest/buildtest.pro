include(../test.pri)
TARGET = buildtest
CONFIG += debug
SOURCES += main.cpp
HEADERS += blog.h
SOURCES += blog.cpp
HEADERS += blogobject.h
HEADERS += foo.h
SOURCES += foo.cpp
HEADERS += fooobject.h
HEADERS += emailmailer.h
SOURCES += emailmailer.cpp
SOURCES += cli.cpp
