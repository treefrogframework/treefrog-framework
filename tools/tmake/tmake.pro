TARGET   = tmake
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      -= gui
MOC_DIR = .obj/
OBJECTS_DIR = .obj/

# C++ Standards Support
CONFIG += c++20
windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-

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
