TARGET = logsenderbench
TEMPLATE = app
CONFIG += console debug qtestlib
CONFIG -= app_bundle
QT += network
QT -= gui
INCLUDEPATH += ../../../../include ../..

SOURCES = benchmarking.cpp \
          ../../tlogsender.cpp \
          ../../tlogfilewriter.cpp

#          ../../tactioncontext.cpp \
#          ../../tactionprocess.cpp \
#          ../../twebapplication.cpp 
