TARGET   = tmake
TEMPLATE = app
VERSION  = 1.0.0
CONFIG  += console c++11
CONFIG  -= app_bundle
QT      -= gui
INCLUDEPATH += ../../include

include(../../tfbase.pri)

unix {
  # c++11
  lessThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=c++0x
  }
}

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
