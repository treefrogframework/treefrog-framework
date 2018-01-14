TARGET = tmaketest
TEMPLATE = app
CONFIG += console debug c++11
CONFIG -= app_bundle
QT += testlib
QT -= gui
INCLUDEPATH += .. ../../../include

SOURCES = tmaketest.cpp \
          ../otmparser.cpp \
          ../otamaconverter.cpp \
          ../viewconverter.cpp \
          ../erbparser.cpp \
          ../erbconverter.cpp \
          ../../../src/thtmlparser.cpp
