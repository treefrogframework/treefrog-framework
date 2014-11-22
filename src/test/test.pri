TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT += network sql
QT -= gui
lessThan(QT_MAJOR_VERSION, 5) {
  CONFIG += qtestlib
} else {
  QT += testlib
}
INCLUDEPATH += ../../../include

include(../../tfbase.pri)
win32|msvc {
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L"..\\..\\debug" -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -L"..\\..\\release" -ltreefrog$${TF_VER_MAJ}
  }
} else:macx {
  LIBS += -F../../ -framework treefrog
} else:unix {
  LIBS += -L../../ -ltreefrog
}
