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
  CONFIG += c++17
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17 /permissive-
}

include(../../tfbase.pri)
INCLUDEPATH += ../../../include  ../.. ../../../3rdparty/glog/build ../../../3rdparty/glog/src

win32 {
  lessThan(QT_MAJOR_VERSION, 6) {
    win32-msvc* {
      QMAKE_CXXFLAGS += /source-charset:utf-8 /wd 4819 /wd 4661
    }
  }
  CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    LIBS += -L../../debug -ltreefrogd$${TF_VER_MAJ} ../../../3rdparty/glog/build/Debug/glogd.lib
  } else {
    LIBS += -L../../release -ltreefrog$${TF_VER_MAJ}
  }
} else:unix {
  LIBS += -Wl,-rpath,../../ -L../../ -ltreefrog ../../../3rdparty/glog/build/libglog.a
  linux-*:LIBS += -lrt $$system("pkg-config --libs libunwind 2>/dev/null")
}
