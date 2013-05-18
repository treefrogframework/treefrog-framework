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

# Erases CR codes on UNIX
!exists(defaults) : system( mkdir defaults )
for(F, defaults.files) {
  win32 {
    F = $$replace(F, /, \\)
    system( COPY /Y ..\\..\\$${F} $${F} > nul )
  }
  unix : system( tr -d "\\\\r" < ../../$${F} > $${F} )
}

contains(CONFIG, 'x86_64') {
  unix {
    system( sed -i -e \'/CONFIG/s/$/ x86_64/\' defaults/controllers.pro )
    system( sed -i -e \'/CONFIG/s/$/ x86_64/\' defaults/models.pro )
    system( sed -i -e \'/CONFIG/s/$/ x86_64/\' defaults/_src.pro )
    system( sed -i -e \'/CONFIG/s/$/ x86_64/\' defaults/helpers.pro )
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
