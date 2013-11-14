TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT += network
QT -= gui
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += testlib
} else {
  CONFIG += qtestlib
}
DEFINES += 
INCLUDEPATH += ../../../include

SOURCES = main.cpp


