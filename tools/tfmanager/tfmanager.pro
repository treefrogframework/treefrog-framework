TARGET = treefrog
TEMPLATE = app
VERSION = 1.0.0
CONFIG += console
CONFIG -= app_bundle
QT     += network
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
  LIBS += -L "$$target.path" -lpsapi
} else:macx {
  INCLUDEPATH += $$header.path
  LIBS += -F$$lib.path -framework treefrog
} else:unix {
  INCLUDEPATH += $$header.path
  LIBS += -L$$lib.path -ltreefrog
}

!isEmpty( use_mongo ) {
  LIBS += -lmongoc
}

isEmpty( target.path ) {
  win32 {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}
INSTALLS += target

DEFINES += TF_VERSION=\\\"$$TF_VERSION\\\"
DEFINES += INSTALL_PATH=\\\"$$target.path\\\"
!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp \
           servermanager.cpp \
           processinfo.cpp

HEADERS += servermanager.h \
           processinfo.h

win32 {
  LIBS += -lws2_32
  SOURCES += processinfo_win.cpp
  SOURCES += windowsservice_win.cpp
}
linux-* {
  SOURCES += processinfo_linux.cpp
}
macx {
  SOURCES += processinfo_macx.cpp
}
