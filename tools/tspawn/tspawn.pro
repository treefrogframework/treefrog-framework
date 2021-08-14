TARGET   = tspawn
TEMPLATE = app
VERSION  = 2.0.0
CONFIG  += console
CONFIG  -= app_bundle
QT      += sql
QT      -= gui
lessThan(QT_MAJOR_VERSION, 6) {
  CONFIG += c++14
  windows:QMAKE_CXXFLAGS += /std:c++14
} else {
  CONFIG += c++17
  QT += core5compat
  windows:QMAKE_CXXFLAGS += /Zc:__cplusplus /std:c++17
}

DEFINES *= QT_USE_QSTRINGBUILDER
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
} else:unix {
  LIBS += -Wl,-rpath,$$lib.path -L$$lib.path -ltreefrog
  linux-*:LIBS += -lrt
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
defaults.files += defaults/cache.ini
defaults.files += defaults/views.pro
# React scripts
defaults.files += defaults/JSXTransformer.js
defaults.files += defaults/react.js
defaults.files += defaults/react-with-addons.js
defaults.files += defaults/react-dom-server.js
# CMake
defaults.files += defaults/CMakeLists.txt
defaults.files += defaults/CacheClean.cmake
defaults.files += defaults/TargetCmake.cmake
defaults.path   = $${datadir}/defaults
lessThan(QT_MAJOR_VERSION, 6) {
  # Qt5
  defaults_controllers.files += defaults/controllers_qt5/CMakeLists.txt
  defaults_controllers.path   = $${datadir}/defaults/controllers
  defaults_models.files += defaults/models_qt5/CMakeLists.txt
  defaults_models.path   = $${datadir}/defaults/models
  defaults_views.files += defaults/views_qt5/CMakeLists.txt
  defaults_views.path   = $${datadir}/defaults/views
  defaults_helpers.files += defaults/helpers_qt5/CMakeLists.txt
  defaults_helpers.path   = $${datadir}/defaults/helpers
} else {
  # Qt6
  defaults_controllers.files += defaults/controllers/CMakeLists.txt
  defaults_controllers.path   = $${datadir}/defaults/controllers
  defaults_models.files += defaults/models/CMakeLists.txt
  defaults_models.path   = $${datadir}/defaults/models
  defaults_views.files += defaults/views/CMakeLists.txt
  defaults_views.path   = $${datadir}/defaults/views
  defaults_helpers.files += defaults/helpers/CMakeLists.txt
  defaults_helpers.path   = $${datadir}/defaults/helpers
}

windows {
  defaults.files += defaults/_dummymodel.h
  defaults.files += defaults/_dummymodel.cpp
}

cmake.files += defaults/cmake/TreeFrogConfig.cmake
cmake.path   = $${datadir}/cmake

INSTALLS += target defaults defaults_controllers defaults_models defaults_views defaults_helpers cmake

windows {
  contains(QMAKE_TARGET.arch, x86_64) {
    clientlib.files += ../../3rdparty/clientlib/win64/COPYING_3RD_PARTY_DLL
    clientlib.files += ../../3rdparty/clientlib/win64/libeay32.dll
    clientlib.files += ../../3rdparty/clientlib/win64/libintl-8.dll
    clientlib.files += ../../3rdparty/clientlib/win64/libmariadb.dll
    clientlib.files += ../../3rdparty/clientlib/win64/libmysql.dll
    clientlib.files += ../../3rdparty/clientlib/win64/libpq.dll
    clientlib.files += ../../3rdparty/clientlib/win64/ssleay32.dll
  } else {
    clientlib.files += ../../3rdparty/clientlib/win32/COPYING_3RD_PARTY_DLL
    clientlib.files += ../../3rdparty/clientlib/win32/intl.dll
    clientlib.files += ../../3rdparty/clientlib/win32/libeay32.dll
    clientlib.files += ../../3rdparty/clientlib/win32/libmariadb.dll
    clientlib.files += ../../3rdparty/clientlib/win32/libmysql.dll
    clientlib.files += ../../3rdparty/clientlib/win32/libpq.dll
    clientlib.files += ../../3rdparty/clientlib/win32/ssleay32.dll
  }
  clientlib.path = $${datadir}/bin
  INSTALLS += clientlib
}


# Erases CR codes on UNIX
!exists(defaults) : system( mkdir defaults )

INS_LIST = defaults.files defaults_controllers.files defaults_models.files defaults_views.files defaults_helpers.files cmake.files
for(T, INS_LIST) {
  for(F, $${T}) {
    windows {
      F = $$replace(F, /, \\)
      DIR = $$dirname(F)
      !exists($${DIR}) : system( mkdir $${DIR} )
      system( COPY ..\\..\\$${F} $${F} > nul )
    }
    unix {
      DIR = $$dirname(F)
      !exists($${DIR}) : system( mkdir -p $${DIR} )
      system( tr -d "\\\\r" < ../../$${F} > $${F} )
    }
  }
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
HEADERS += servicegenerator.h
SOURCES += servicegenerator.cpp
HEADERS += vueservicegenerator.h
SOURCES += vueservicegenerator.cpp
HEADERS += vueerbgenerator.h
SOURCES += vueerbgenerator.cpp
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
HEADERS += helpergenerator.h
SOURCES += helpergenerator.cpp
HEADERS += apicontrollergenerator.h
SOURCES += apicontrollergenerator.cpp
HEADERS += apiservicegenerator.h
SOURCES += apiservicegenerator.cpp
HEADERS += util.h
SOURCES += util.cpp
