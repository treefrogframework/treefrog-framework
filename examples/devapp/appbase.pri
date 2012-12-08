win32 {
  INCLUDEPATH += $$quote($$(TFDIR)\\include)
  LIBS += -L$$quote($$(TFDIR)\\bin)
  CONFIG(debug, debug|release) {
    LIBS += -ltreefrogd1
  } else {
    LIBS += -ltreefrog1
  }
} else:macx {
  INCLUDEPATH += /Library/Frameworks/treefrog.framework/Versions/Current/Headers
  LIBS += -framework treefrog
} else:unix {
  INCLUDEPATH += ../../include /usr/include/treefrog
  LIBS += -ltreefrog
}
