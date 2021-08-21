TARGET   = treefrog
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      += network
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
  LIBS += -L"$$target.path" -lntdll
  win32-msvc* {
    LIBS += advapi32.lib
  }
} else:unix {
  LIBS += -Wl,-rpath,$$lib.path -L$$lib.path -ltreefrog
  linux-*:LIBS += -lrt
}

INSTALLS += target

DEFINES += TF_VERSION=\\\"$$TF_VERSION\\\"
DEFINES += INSTALL_PATH=\\\"$$target.path\\\"
!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp \
           servermanager.cpp \
           systembusdaemon.cpp

HEADERS += servermanager.h \
           systembusdaemon.h

windows {
  LIBS += -lws2_32
  SOURCES += windowsservice_win.cpp
}
