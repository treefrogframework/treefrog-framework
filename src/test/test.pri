TEMPLATE = app
CONFIG += console c++11 testcase
CONFIG -= app_bundle
QT += network sql
QT -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += qml
DEFINES += TF_DLL

lessThan(QT_MAJOR_VERSION, 5) {
  CONFIG += qtestlib
} else {
  QT += testlib
}

include(../../tfbase.pri)
INCLUDEPATH += ../../../include  ../..

win32 {
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L../../debug -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -L../../release -ltreefrog$${TF_VER_MAJ}
  }
} else:unix {
  LIBS += -Wl,-rpath,../../ -L../../ -ltreefrog

  # c++11
  lessThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=c++0x
  }
}
