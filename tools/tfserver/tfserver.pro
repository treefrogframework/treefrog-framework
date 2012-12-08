TARGET = tadpole
TEMPLATE = app
VERSION = 1.0.0
CONFIG += console
CONFIG -= app_bundle
QT     += network sql
QT     -= gui
DEFINES += TF_DLL

include(../../tfbase.pri)

win32 {
  INCLUDEPATH += $$header.path
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  LIBS += -L "$$target.path"
} else:macx {
  INCLUDEPATH += $$header.path
  LIBS += -F$$lib.path -framework treefrog
} else:unix {
  INCLUDEPATH += $$header.path
  LIBS += -L$$lib.path -ltreefrog
}

isEmpty( target.path ) {
  win32 {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}
INSTALLS += target

!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp

unix {
  HEADERS += signalhandler.h
  SOURCES += signalhandler.cpp
  HEADERS += symbolize.h
  SOURCES += symbolize.cpp
  HEADERS += demangle.h
  SOURCES += demangle.cpp
  HEADERS += stacktrace.h
  SOURCES += stacktrace.cpp
  HEADERS += gconfig.h \
             stacktrace_generic-inl.h \
             stacktrace_libunwind-inl.h \
             stacktrace_powerpc-inl.h \
             stacktrace_x86-inl.h \
             stacktrace_x86_64-inl.h
}
