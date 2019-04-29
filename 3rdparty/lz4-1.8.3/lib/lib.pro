TEMPLATE = lib
TARGET   = lz4
CONFIG  += static console release
INCLUDEPATH += .

# Input
HEADERS += lz4.h \
           lz4frame.h \
           lz4frame_static.h \
           lz4hc.h \
           xxhash.h \
           xxhash.c \
           lz4.c
SOURCES += lz4.c \
           lz4frame.c \
           lz4hc.c \
           xxhash.c
