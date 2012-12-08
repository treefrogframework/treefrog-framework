TARGET = htmlparser
TEMPLATE = app
CONFIG += console debug qtestlib
CONFIG -= app_bundle
QT -= gui
INCLUDEPATH += ../../../include
SOURCES = htmlparser.cpp


include(../../../tfbase.pri)
win32 {
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L "..\\..\\debug" -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -L "..\\..\\release" -ltreefrog$${TF_VER_MAJ}
  }
} else:macx {
  LIBS += -F../../ -framework treefrog
} else:unix {
  LIBS += -L../../ -ltreefrog
}

