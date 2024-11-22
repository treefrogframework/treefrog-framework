TARGET = tmaketest
TEMPLATE = app
CONFIG += console debug
CONFIG -= app_bundle
QT += testlib
QT -= gui

# C++ Standards Support
CONFIG += c++20
windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-

INCLUDEPATH += .. ../../../include

SOURCES = tmaketest.cpp \
          ../otmparser.cpp \
          ../otamaconverter.cpp \
          ../viewconverter.cpp \
          ../erbparser.cpp \
          ../erbconverter.cpp \
          ../../../src/thtmlparser.cpp
