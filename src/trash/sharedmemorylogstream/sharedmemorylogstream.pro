include(../test.pri)
TARGET = sharedmemorylogstream
SOURCES += benchmarking.cpp


###
HEADERS += ../../trash/tfileaiologger.h
SOURCES += ../../trash/tfileaiologger.cpp
HEADERS += ../../trash/tfileaiowriter.h
SOURCES += ../../trash/tfileaiowriter.cpp

windows {
  SOURCES += ../../trash/tfileaiowriter_win.cpp
} else {
  SOURCES += ../../trash/tfileaiowriter_unix.cpp
}
