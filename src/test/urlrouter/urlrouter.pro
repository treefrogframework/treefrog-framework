TARGET = urlrouter
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= gui
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += testlib
} else {
  CONFIG += qtestlib
}
DEFINES +=
INCLUDEPATH += ../../../include
INCLUDEPATH += ../../../src
SOURCES = main.cpp


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

