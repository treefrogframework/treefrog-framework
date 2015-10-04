win32 {
  INCLUDEPATH += $$quote($$(TFDIR)\\include)
  LIBS += -L$$quote($$(TFDIR)\\bin)
  CONFIG(debug, debug|release) {
    LIBS += -ltreefrogd1
  } else {
    LIBS += -ltreefrog1
  }
} else {
  macx {
    LIBS += -F/Library/Frameworks
    LIBS += -framework treefrog
    macx:INCLUDEPATH += /Library/Frameworks/treefrog.framework/Headers
  } else {
    LIBS += -ltreefrog
    unix:INCLUDEPATH += /usr/include/treefrog
  }

  # c++11
  lessThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=c++0x
  }
}
