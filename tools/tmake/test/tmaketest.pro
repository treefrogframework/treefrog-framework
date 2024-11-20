TARGET = tmaketest
TEMPLATE = app
CONFIG += console debug
CONFIG -= app_bundle
QT += testlib
QT -= gui
lessThan(QT_MINOR_VERSION, 3) {
  # Qt6.2
  CONFIG += c++17
  windows:QMAKE_CXXFLAGS += /std:c++17
} else {
  CONFIG += c++20
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-
}

INCLUDEPATH += .. ../../../include

SOURCES = tmaketest.cpp \
          ../otmparser.cpp \
          ../otamaconverter.cpp \
          ../viewconverter.cpp \
          ../erbparser.cpp \
          ../erbconverter.cpp \
          ../../../src/thtmlparser.cpp
