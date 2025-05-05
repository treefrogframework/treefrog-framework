TARGET   = tadpole
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      += network sql xml qml
QT      -= gui
MOC_DIR = .obj/
OBJECTS_DIR = .obj/

# C++ Standards Support
CONFIG += c++20
windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-

DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES += TF_DLL GLOG_USE_GLOG_EXPORT
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
  # for windows
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -ltreefrogd$${TF_VER_MAJ} ../../3rdparty/glog/build/Debug/glogd.lib dbghelp.lib
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  INCLUDEPATH += ../../3rdparty/glog/build ../../3rdparty/glog/src
  LIBS += -L"$$target.path"
} else {
  LIBS += -Wl,-rpath,$$lib.path -L$$lib.path -ltreefrog
  # glog
  isEmpty( enable_shared_glog ) {
    # static link
    LIBS += ../../3rdparty/glog/build/libglog.a $$system("pkg-config --libs gflags 2>/dev/null")
    INCLUDEPATH += ../../3rdparty/glog/build ../../3rdparty/glog/src
  } else {
    # shared link '-lglog'
    LIBS += $$system("pkg-config --libs libglog 2>/dev/null")
  }
  # for linux
  linux-* {
    # -lunwind
    LIBS += -lrt $$system("pkg-config --libs libunwind 2>/dev/null")
  }
}

INSTALLS += target

!CONFIG(debug, debug|release) {
  DEFINES += TF_NO_DEBUG
}

SOURCES += main.cpp
