TARGET   = tmake
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      -= gui
lessThan(QT_MAJOR_VERSION, 6) {
  CONFIG += c++14
  windows:QMAKE_CXXFLAGS += /std:c++14
} else {
  CONFIG += c++17
  QT += core5compat
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17
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
