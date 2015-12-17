TARGET   = tspawn
TEMPLATE = app
VERSION  = 1.0.0
CONFIG  += console c++11
CONFIG  -= app_bundle
QT      += sql
QT      -= gui
DEFINES += TF_DLL
INCLUDEPATH += $$header.path

include(../../tfbase.pri)

isEmpty( datadir ) {
  windows {
    datadir = C:/TreeFrog/$${TF_VERSION}
  } else {
    datadir = /usr/share/treefrog
  }
}
DEFINES += TREEFROG_DATA_DIR=\\\"$$datadir\\\"

windows {
  CONFIG(debug, debug|release) {
    LIBS += -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  LIBS += -L"$$target.path"
} else:macx {
  LIBS += -F$$lib.path -framework treefrog
} else:unix {
  LIBS += -L$$lib.path -ltreefrog

  # c++11
  lessThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=c++0x
  }
}

isEmpty( target.path ) {
  windows {
    target.path = C:/TreeFrog/$${TF_VERSION}/bin
  } else {
    target.path = /usr/bin
  }
}

defaults.files  = defaults/.trim_mode
defaults.files += defaults/403.html
defaults.files += defaults/404.html
defaults.files += defaults/413.html
defaults.files += defaults/500.html
defaults.files += defaults/_src.pro
defaults.files += defaults/app.pro
defaults.files += defaults/appbase.pri
defaults.files += defaults/application.ini
defaults.files += defaults/applicationcontroller.cpp
defaults.files += defaults/applicationcontroller.h
defaults.files += defaults/applicationhelper.cpp
defaults.files += defaults/applicationhelper.h
defaults.files += defaults/applicationendpoint.cpp
defaults.files += defaults/applicationendpoint.h
defaults.files += defaults/controllers.pro
defaults.files += defaults/database.ini
defaults.files += defaults/development.ini
defaults.files += defaults/helpers.pro
defaults.files += defaults/internet_media_types.ini
defaults.files += defaults/logger.ini
defaults.files += defaults/mail.erb
defaults.files += defaults/models.pro
defaults.files += defaults/mongodb.ini
defaults.files += defaults/redis.ini
defaults.files += defaults/routes.cfg
defaults.files += defaults/validation.ini
defaults.files += defaults/views.pro
windows {
  defaults.files += defaults/_dummymodel.h
  defaults.files += defaults/_dummymodel.cpp
}
defaults.path = $${datadir}/defaults
INSTALLS += target defaults

windows {
  contains(QMAKE_TARGET.arch, x86_64) {
    clientlib.files += ../../sqldrivers/clientlib64/COPYING_3RD_PARTY_DLL
    clientlib.files += ../../sqldrivers/clientlib64/libeay32.dll
    clientlib.files += ../../sqldrivers/clientlib64/libintl-8.dll
    clientlib.files += ../../sqldrivers/clientlib64/libmariadb.dll
    clientlib.files += ../../sqldrivers/clientlib64/libmysql.dll
    clientlib.files += ../../sqldrivers/clientlib64/libpq.dll
    clientlib.files += ../../sqldrivers/clientlib64/ssleay32.dll
  } else {
    clientlib.files += ../../sqldrivers/clientlib32/COPYING_3RD_PARTY_DLL
    clientlib.files += ../../sqldrivers/clientlib32/intl.dll
    clientlib.files += ../../sqldrivers/clientlib32/libeay32.dll
    clientlib.files += ../../sqldrivers/clientlib32/libmariadb.dll
    clientlib.files += ../../sqldrivers/clientlib32/libmysql.dll
    clientlib.files += ../../sqldrivers/clientlib32/libpq.dll
    clientlib.files += ../../sqldrivers/clientlib32/ssleay32.dll
  }
  clientlib.path = $${datadir}/bin
  INSTALLS += clientlib
}


# Erases CR codes on UNIX
!exists(defaults) : system( mkdir defaults )
for(F, defaults.files) {
  windows {
    F = $$replace(F, /, \\)
    system( COPY /Y ..\\..\\$${F} $${F} > nul )
  }
  unix : system( tr -d "\\\\r" < ../../$${F} > $${F} )
}

unix {
  contains(CONFIG, 'x86_64') {
    mac : EXT=\'\'
    system( sed -i $$EXT -e \'/CONFIG/s/$/ x86_64/\' defaults/controllers.pro )
    system( sed -i $$EXT -e \'/CONFIG/s/$/ x86_64/\' defaults/models.pro )
    system( sed -i $$EXT -e \'/CONFIG/s/$/ x86_64/\' defaults/_src.pro )
    system( sed -i $$EXT -e \'/CONFIG/s/$/ x86_64/\' defaults/helpers.pro )
  }
}

# Source files
SOURCES += main.cpp
HEADERS += global.h
SOURCES += global.cpp
HEADERS += filewriter.h
SOURCES += filewriter.cpp
HEADERS += tableschema.h
SOURCES += tableschema.cpp
HEADERS += projectfilegenerator.h
SOURCES += projectfilegenerator.cpp
HEADERS += controllergenerator.h
SOURCES += controllergenerator.cpp
HEADERS += modelgenerator.h
SOURCES += modelgenerator.cpp
HEADERS += abstractobjgenerator.h
SOURCES += abstractobjgenerator.cpp
HEADERS += sqlobjgenerator.h
SOURCES += sqlobjgenerator.cpp
HEADERS += mongoobjgenerator.h
SOURCES += mongoobjgenerator.cpp
HEADERS += websocketgenerator.h
SOURCES += websocketgenerator.cpp
HEADERS += validatorgenerator.h
SOURCES += validatorgenerator.cpp
HEADERS += otamagenerator.h
SOURCES += otamagenerator.cpp
HEADERS += erbgenerator.h
SOURCES += erbgenerator.cpp
HEADERS += mailergenerator.h
SOURCES += mailergenerator.cpp
HEADERS += mongocommand.h
SOURCES += mongocommand.cpp
HEADERS += util.h
SOURCES += util.cpp
