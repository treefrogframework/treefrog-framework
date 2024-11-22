TEMPLATE = app
CONFIG += console testcase
CONFIG -= app_bundle
QT += network sql qml testlib
QT -= gui
DEFINES += TF_DLL

# C++ Standards Support
CONFIG += c++20
windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-

include(../../tfbase.pri)
INCLUDEPATH += ../../../include ../..

windows {
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L../../debug -ltreefrogd$${TF_VER_MAJ} ../../../3rdparty/glog/build/Debug/glogd.lib dbghelp.lib
  } else {
    LIBS += -L../../release -ltreefrog$${TF_VER_MAJ}
  }
  INCLUDEPATH += ../../../3rdparty/glog/build ../../../3rdparty/glog/src
} else:unix {
  LIBS += -Wl,-rpath,../../ -L../../ -ltreefrog
  exists(../../3rdparty/glog/build/libglog.a) {
    # static link
    LIBS += ../../../3rdparty/glog/build/libglog.a $$system("pkg-config --libs gflags 2>/dev/null")
    INCLUDEPATH += ../../../3rdparty/glog/build ../../../3rdparty/glog/src
  } else {
    # shared link '-lglog'
    LIBS += $$system("pkg-config --libs libglog 2>/dev/null")
  }
  linux-*:LIBS += -lrt $$system("pkg-config --libs libunwind 2>/dev/null")
}
