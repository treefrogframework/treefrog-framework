TARGET = buildtest
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT += network sql
QT -= gui
DEFINES += 
INCLUDEPATH += ../../../include

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

SOURCES += main.cpp
HEADERS += blog.h
SOURCES += blog.cpp
HEADERS += blogobject.h
HEADERS += foo.h
SOURCES += foo.cpp
HEADERS += fooobject.h

