TARGET   = tadpole
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      += network sql xml qml
QT      -= gui
lessThan(QT_MAJOR_VERSION, 6) {
  CONFIG += c++14
  windows:QMAKE_CXXFLAGS += /std:c++14
} else {
  CONFIG += c++17
  QT += core5compat
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17
}

DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES += TF_DLL
INCLUDEPATH += $$header.path

include(../../tfbase.pri)

isEmpty( target.path ) {
  windows {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}

windows {
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  LIBS += -L"$$target.path"
} else:unix {
  LIBS += -Wl,-rpath,$$lib.path -L$$lib.path -ltreefrog
  linux-*:LIBS += -lrt
  *-g++:DEFINES += _GNU_SOURCE
  freebsd-*:DEFINES += _GNU_SOURCE
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
