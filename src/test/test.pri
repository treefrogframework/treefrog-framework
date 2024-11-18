TEMPLATE = app
CONFIG += console testcase
CONFIG -= app_bundle
QT += network sql qml testlib
QT -= gui
DEFINES += TF_DLL

lessThan(QT_MAJOR_VERSION, 6) {
  CONFIG += c++14
  windows:QMAKE_CXXFLAGS += /std:c++14
} else {
  CONFIG += c++20
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20 /permissive-
}

include(../../tfbase.pri)
INCLUDEPATH += ../../../include ../..

win32 {
  lessThan(QT_MAJOR_VERSION, 6) {
    win32-msvc* {
      QMAKE_CXXFLAGS += /source-charset:utf-8 /wd 4819 /wd 4661
    }
  }
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
