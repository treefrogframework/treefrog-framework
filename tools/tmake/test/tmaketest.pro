TARGET = tmaketest
TEMPLATE = app
CONFIG += console debug c++11
CONFIG -= app_bundle
QT -= gui
INCLUDEPATH += .. ../../../include

lessThan(QT_MAJOR_VERSION, 5) {
  CONFIG += qtestlib
} else {
  QT += testlib
}

SOURCES = tmaketest.cpp \
          ../otmparser.cpp \
          ../otamaconverter.cpp \
          ../viewconverter.cpp \
          ../erbparser.cpp \
          ../erbconverter.cpp \
          ../../../src/thtmlparser.cpp
