TARGET = tspawn
TEMPLATE = app
VERSION = 1.0.0
CONFIG += console
CONFIG -= app_bundle
QT += sql
QT -= gui

include(../../tfbase.pri)
INCLUDEPATH += $$header.path

isEmpty( datadir ) {
  win32 {
    datadir = C:/TreeFrog/$${TF_VERSION}
  } else {
    datadir = /usr/share/treefrog
  }
}
DEFINES += TREEFROG_DATA_DIR=\\\"$$datadir\\\"

!isEmpty( use_mongo ) {
  DEFINES += TF_BUILD_MONGODB
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
  contains(QT_VERSION, ^5\\.0\\..*) {
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/install_sqldrivers.bat
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/LGPL_EXCEPTION.txt
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/LICENSE.FDL
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/LICENSE.GPL
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/LICENSE.LGPL
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/LICENSE.PREVIEW.COMMERCIAL
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/README
    sqldrivers.files += ../../sqldrivers/qt5.0-mingw/README.ja
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqldb2.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqldb2d.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqlmysql.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqlmysqld.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqloci.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqlocid.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqlpsql.dll
    drivers.files += ../../sqldrivers/qt5.0-mingw/drivers/qsqlpsqld.dll
  } else {
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/install_sqldrivers.bat
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/LGPL_EXCEPTION.txt
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/LICENSE.FDL
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/LICENSE.GPL
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/LICENSE.LGPL
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/LICENSE.PREVIEW.COMMERCIAL
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/README
    sqldrivers.files += ../../sqldrivers/qt5.1-mingw/README.ja
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqldb2.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqldb2d.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqlmysql.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqlmysqld.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqloci.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqlocid.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqlpsql.dll
    drivers.files += ../../sqldrivers/qt5.1-mingw/drivers/qsqlpsqld.dll
  }
  sqldrivers.path = $${datadir}/sqldrivers
  drivers.path = $${datadir}/sqldrivers/drivers
  INSTALLS += sqldrivers drivers

  clientlib.files += ../../sqldrivers/clientlib/libintl.dll
  clientlib.files += ../../sqldrivers/clientlib/libmariadb.dll
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

HEADERS = global.h \
          filewriter.h \
          tableschema.h \
          projectfilegenerator.h \
          controllergenerator.h \
          modelgenerator.h \
          validatorgenerator.h \
          otamagenerator.h \
          erbgenerator.h \
          mailergenerator.h \
          util.h

SOURCES = main.cpp \
          global.cpp \
          filewriter.cpp \
          tableschema.cpp \
          projectfilegenerator.cpp \
          controllergenerator.cpp \
          modelgenerator.cpp \
          validatorgenerator.cpp \
          otamagenerator.cpp \
          erbgenerator.cpp \
          mailergenerator.cpp \
          util.cpp
