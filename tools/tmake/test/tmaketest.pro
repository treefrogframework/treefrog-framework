TARGET = tmaketest
TEMPLATE = app
CONFIG += console debug qtestlib
CONFIG -= app_bundle
QT -= gui
INCLUDEPATH += .. ../../../include

SOURCES = tmaketest.cpp \
          ../otmparser.cpp \
          ../otamaconverter.cpp \
          ../viewconverter.cpp \
          ../erbparser.cpp \
          ../erbconverter.cpp \
          ../../../src/thtmlparser.cpp
