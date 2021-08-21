TARGET = view
TEMPLATE = lib
CONFIG += shared
QT += network xml qml
QT -= gui
DEFINES += TF_DLL
DESTDIR = ../../lib
INCLUDEPATH += ../../helpers ../../models
DEPENDPATH  += ../../helpers ../../models
LIBS += -L../../lib -lhelper -lmodel
MOC_DIR = .obj/
OBJECTS_DIR = .obj/
QMAKE_CLEAN = *.cpp *.moc *.o source.list

tmake.target = source.list
tmake.commands = tmake -f ../../config/application.ini -v .. -d . -P
tmake.depends = qmake_all
QMAKE_EXTRA_TARGETS = tmake
POST_TARGETDEPS = source.list

include(../../appbase.pri)
!exists(source.list) {
  system( $$tmake.commands )
}
include(source.list)
