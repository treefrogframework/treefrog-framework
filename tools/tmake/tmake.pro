TARGET   = tmake
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      -= gui
lessThan(QT_MINOR_VERSION, 3) {
  # Qt6.2
  CONFIG += c++17
  windows:QMAKE_CXXFLAGS += /std:c++17
} else {
  CONFIG += c++20
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-
}

INCLUDEPATH += ../../include
DEFINES *= QT_USE_QSTRINGBUILDER

include(../../tfbase.pri)

isEmpty( target.path ) {
  windows {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}
INSTALLS += target

HEADERS = viewconverter.h \
          erbconverter.h \
          erbparser.h \
          otmparser.h \
          otamaconverter.h \
          ../../src/thtmlparser.h
SOURCES = main.cpp \
          viewconverter.cpp \
          erbconverter.cpp \
          erbparser.cpp \
          otmparser.cpp \
          otamaconverter.cpp \
          ../../src/thtmlparser.cpp
