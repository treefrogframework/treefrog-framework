TARGET = view
TEMPLATE = lib
CONFIG += shared c++11
QT += network xml qml
QT -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += qml
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
lessThan(QT_MAJOR_VERSION, 5) {
  tmake.depends = qmake
} else {
  tmake.depends = qmake_all
}
QMAKE_EXTRA_TARGETS = tmake
POST_TARGETDEPS = source.list

include(../../appbase.pri)
!exists(source.list) {
  system( $$tmake.commands )
}
include(source.list)
