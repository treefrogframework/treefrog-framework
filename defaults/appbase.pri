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
  } else {
    LIBS += -ltreefrog
  }
  unix:INCLUDEPATH += /Library/Frameworks/treefrog.framework/Headers
}
