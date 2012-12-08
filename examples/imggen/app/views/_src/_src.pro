TARGET = view
TEMPLATE = lib
CONFIG += shared
QT -= gui
QT += network
DEFINES += TF_DLL
INCLUDEPATH += ../../helpers ../../models
DEPENDPATH  += ../../helpers ../../models
DESTDIR = ../../../lib
LIBS += -L../../../lib -lhelper
QMAKE_CLEAN = *.cpp source.list

tmake.target = source.list
tmake.commands = tmake -f ../../../config/treefrog.ini -v .. -d . -P
tmake.depends = qmake
QMAKE_EXTRA_TARGETS = tmake

include(../../appbase.pri)
!exists(source.list) {
  system( $$tmake.commands )
}
include(source.list)
