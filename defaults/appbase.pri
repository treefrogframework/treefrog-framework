CONFIG += c++20
windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++20

windows {
  INCLUDEPATH += $$quote($$(TFDIR)\\include)
  LIBS += -L$$quote($$(TFDIR)\\bin)
  CONFIG(debug, debug|release) {
    LIBS += -ltreefrogd2
  } else {
    LIBS += -ltreefrog2
  }
} else {
  unix:LIBS += -Wl,-rpath,. -Wl,-rpath,/usr/lib -L/usr/lib -ltreefrog
  unix:INCLUDEPATH += /usr/include/treefrog
  linux-*:LIBS += -lrt
}
