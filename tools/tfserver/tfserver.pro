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
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17 /permissive-
}

DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES += TF_DLL
INCLUDEPATH += $$header.path ../../3rdparty/glog/build ../../3rdparty/glog/src

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
    LIBS += -ltreefrogd$${TF_VER_MAJ} ../../3rdparty/glog/build/Debug/glogd.lib
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  LIBS += -L"$$target.path"
} else:unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += libunwind
  LIBS += -Wl,-rpath,$$lib.path -L$$lib.path -ltreefrog ../../3rdparty/glog/build/libglog.a
  linux-*:LIBS += -lrt
}

INSTALLS += target

!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp
