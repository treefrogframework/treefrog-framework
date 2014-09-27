#
# MongoDB C driver project file
#

TEMPLATE = lib
TARGET = mongoc
CONFIG += static console
CONFIG -= qt
DEFINES += MONGO_HAVE_STDINT MONGO_DLL_BUILD _POSIX_SOURCE MONGO_STATIC_BUILD
macx:DEFINES += _DARWIN_C_SOURCE
*-g++|*-clang {
  QMAKE_CFLAGS += -std=c99
}
DEPENDPATH += src
INCLUDEPATH += src

# Input
HEADERS += src/bcon.h \
           src/bson.h \
           src/encoding.h \
           src/env.h \
           src/gridfs.h \
           src/md5.h \
           src/mongo.h \
           test/test.h
SOURCES += src/bcon.c \
           src/bson.c \
           src/encoding.c \
           src/env.c \
           src/gridfs.c \
           src/md5.c \
           src/mongo.c \
           src/numbers.c
