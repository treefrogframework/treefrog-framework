TARGET   = tspawn
TEMPLATE = app
VERSION  = 1.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      += sql
QT      -= gui
DEFINES += TF_DLL
INCLUDEPATH += $$header.path

include(../../tfbase.pri)

isEmpty( datadir ) {
  win32 {
    datadir = C:/TreeFrog/$${TF_VERSION}
  } else {
    datadir = /usr/share/treefrog
  }
}
DEFINES += TREEFROG_DATA_DIR=\\\"$$datadir\\\"

win32 {
  CONFIG(debug, debug|release) {
    LIBS += -ltreefrogd$${TF_VER_MAJ}
  } else {
    LIBS += -ltreefrog$${TF_VER_MAJ}
  }
  LIBS += -L "$$target.path"
} else:macx {
  LIBS += -F$$lib.path -framework treefrog
} else:unix {
  LIBS += -L$$lib.path -ltreefrog
}

isEmpty( target.path ) {
  win32 {
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
defaults.files += defaults/controllers.pro
defaults.files += defaults/database.ini
defaults.files += defaults/development.ini
defaults.files += defaults/helpers.pro
defaults.files += defaults/internet_media_types.ini
defaults.files += defaults/logger.ini
defaults.files += defaults/mail.erb
defaults.files += defaults/models.pro
defaults.files += defaults/mongodb.ini
defaults.files += defaults/routes.cfg
defaults.files += defaults/validation.ini
defaults.files += defaults/views.pro
defaults.path = $${datadir}/defaults
INSTALLS += target defaults

win32 {
  sqldrivers.files += ../../sqldrivers/qt5-mingw/install_sqldrivers.bat
  sqldrivers.files += ../../sqldrivers/qt5-mingw/LGPL_EXCEPTION.txt
  sqldrivers.files += ../../sqldrivers/qt5-mingw/LICENSE.FDL
  sqldrivers.files += ../../sqldrivers/qt5-mingw/LICENSE.GPL
  sqldrivers.files += ../../sqldrivers/qt5-mingw/LICENSE.LGPL
  sqldrivers.files += ../../sqldrivers/qt5-mingw/LICENSE.PREVIEW.COMMERCIAL
  sqldrivers.files += ../../sqldrivers/qt5-mingw/README
  sqldrivers.files += ../../sqldrivers/qt5-mingw/README.ja
  sqldrivers.path = $${datadir}/sqldrivers
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqldb2.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqldb2d.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqlmysql.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqlmysqld.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqloci.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqlocid.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqlpsql.dll
  drivers502.files += ../../sqldrivers/qt5-mingw/5.0.2/qsqlpsqld.dll
  drivers502.path = $${datadir}/sqldrivers/5.0.2
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqldb2.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqldb2d.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqlmysql.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqlmysqld.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqloci.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqlocid.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqlpsql.dll
  drivers510.files += ../../sqldrivers/qt5-mingw/5.1.0/qsqlpsqld.dll
  drivers510.path = $${datadir}/sqldrivers/5.1.0
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqldb2.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqldb2d.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqlmysql.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqlmysqld.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqloci.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqlocid.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqlpsql.dll
  drivers511.files += ../../sqldrivers/qt5-mingw/5.1.1/qsqlpsqld.dll
  drivers511.path = $${datadir}/sqldrivers/5.1.1

  INSTALLS += sqldrivers drivers502 drivers510 drivers511

  clientlib.files += ../../sqldrivers/clientlib/COPYING_3RD_PARTY_DLL
  clientlib.files += ../../sqldrivers/clientlib/libintl.dll
  clientlib.files += ../../sqldrivers/clientlib/libmariadb.dll
  clientlib.files += ../../sqldrivers/clientlib/libmysql.dll
  clientlib.files += ../../sqldrivers/clientlib/libpq.dll
  clientlib.path = $${datadir}/bin
  INSTALLS += clientlib
}


# Erases CR codes on UNIX
!exists(defaults) : system( mkdir defaults )
for(F, defaults.files) {
  win32 {
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
