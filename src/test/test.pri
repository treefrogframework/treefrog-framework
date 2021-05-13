TEMPLATE = app
CONFIG += console c++14 testcase
CONFIG -= app_bundle
QT += network sql qml testlib
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
QT -= gui
DEFINES += TF_DLL

include(../../tfbase.pri)
INCLUDEPATH += ../../../include  ../..

win32 {
  win32-msvc* {
    QMAKE_CXXFLAGS += /source-charset:utf-8 /wd 4819 /wd 4661
  }
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L../../debug -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -L../../release -ltreefrog$${TF_VER_MAJ}
  }
} else:unix {
  LIBS += -Wl,-rpath,../../ -L../../ -ltreefrog
  linux-*:LIBS += -lrt
}
