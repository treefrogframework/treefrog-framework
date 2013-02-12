/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <unistd.h>
#include <time.h>
#include <QtCore>
#include "global.h"
#include "controllergenerator.h"
#include "modelgenerator.h"
#include "otamagenerator.h"
#include "erbgenerator.h"
#include "validatorgenerator.h"
#include "mailergenerator.h"
#include "projectfilegenerator.h"
#include "tableschema.h"
#include "util.h"

#define L(str)  QLatin1String(str)
#define SEP   QDir::separator()
#define P_CTRLS   (QLatin1String("controllers") + SEP)
#define P_MODELS  (QLatin1String("models") + SEP)
#define P_VIEWS   (QLatin1String("views") + SEP)
#define P_HELPERS (QLatin1String("helpers") + SEP)

enum SubCommand {
    Invalid = 0,
    Help,
    New,
    Controller,
    Model,
    UserModel,
    SqlObject,
    Validator,
    Mailer,
    Scaffold,
    Delete,
    ShowDrivers,
    ShowDriverPath,
    ShowTables,
};

typedef QHash<QString, int> StringHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(StringHash, subCommands,
{
    x->insert("-h", Help);
    x->insert("--help", Help);
    x->insert("new", New);
    x->insert("n", New);
    x->insert("controller", Controller);
    x->insert("c", Controller);
    x->insert("model", Model);
    x->insert("m", Model);
    x->insert("usermodel", UserModel);
    x->insert("u", UserModel);
    x->insert("sqlobject", SqlObject);
    x->insert("o", SqlObject);
    x->insert("validator", Validator);
    x->insert("v", Validator);
    x->insert("mailer", Mailer);
    x->insert("l", Mailer);
    x->insert("scaffold", Scaffold);
    x->insert("s", Scaffold);
    x->insert("delete", Delete);
    x->insert("d", Delete);
    x->insert("remove", Delete);
    x->insert("r", Delete);
    x->insert("--show-drivers", ShowDrivers);
    x->insert("--show-driver-path", ShowDriverPath);
    x->insert("--show-tables", ShowTables);
})

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, subDirs,
{
    *x << L("controllers")
       << L("models")
       << L("models") + SEP + "sqlobjects"
       << L("views")
       << L("views") + SEP + "layouts"
       << L("views") + SEP + "mailer"
       << L("views") + SEP + "partial"
       << L("views") + SEP + "_src"
       << L("helpers")
       << L("config") 
       << L("config") + SEP + "environments"
       << L("config") + SEP + "initializers"
       << L("public")
       << L("public") + SEP + "images"
       << L("public") + SEP + "js"
       << L("public") + SEP + "css"
       << L("db") << L("lib") << L("log") << L("plugin")
       << L("script") << L("sql") << L("test") << L("tmp");
})

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, filePaths,
{
    *x << L("appbase.pri")
       << L("controllers") + SEP + "applicationcontroller.h"
       << L("controllers") + SEP + "applicationcontroller.cpp"
       << L("controllers") + SEP + "controllers.pro"
       << L("models") + SEP + "models.pro"
       << L("views") + SEP + "views.pro"
       << L("views") + SEP + "_src" + SEP + "_src.pro"
       << L("views") + SEP + "mailer" + SEP + ".trim_mode"
       << L("helpers") + SEP + "helpers.pro"
       << L("helpers") + SEP + "applicationhelper.h"
       << L("helpers") + SEP + "applicationhelper.cpp"
       << L("config") + SEP + "application.ini"
       << L("config") + SEP + "database.ini"
       << L("config") + SEP + "development.ini"
       << L("config") + SEP + "logger.ini"
       << L("config") + SEP + "routes.cfg"
       << L("config") + SEP + "validation.ini"
       << L("config") + SEP + "initializers" + SEP + "internet_media_types.ini"
       << L("public") + SEP + "403.html"
       << L("public") + SEP + "404.html"
       << L("public") + SEP + "413.html"
       << L("public") + SEP + "500.html"
       << L("script") + SEP + "starttreefrog.bat";
})

const QString appIni = QLatin1String("config") + QDir::separator() + "application.ini";
const QString devIni = QLatin1String("config") + QDir::separator() + "development.ini";
static QSettings appSettings(appIni, QSettings::IniFormat);
static QSettings devSettings(devIni, QSettings::IniFormat);
static QString templateSystem;


static void usage()
{
    qDebug("usage: tspawn <subcommand> [args]\n\n" \
           "Type 'tspawn --show-drivers' to show all the available database drivers for Qt.\n" \
           "Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.\n" \
           "Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.\n\n" \
           "Available subcommands:\n" \
           "  new (n)        <application-name>\n" \
           "  scaffold (s)   <table-name> [model-name]\n" \
           "  controller (c) <controller-name> [action ...]\n" \
           "  model (m)      <table-name> [model-name]\n" \
           "  usermodel (u)  <table-name> [username password [model-name]]\n" \
           "  sqlobject (o)  <table-name> [model-name]\n" \
           "  validator (v)  <name>\n" \
           "  mailer (l)     <mailer-name> action [action ...]\n" \
           "  delete (d)     <table-name or validator-name>\n");
}


static QStringList rmfiles(const QStringList &files, bool &allRemove, bool &quit, const QString &proj = QString())
{
    QStringList rmd;
    
    // Removes files
    for (QStringListIterator i(files); i.hasNext(); ) {
        if (quit)
            break;

        QFile file(i.next());
        if (!file.exists())
            continue;
        
        if (allRemove) {
            rmd << remove(file);
            continue;
        }
        
        QTextStream stream(stdin);
        for (;;) {
            printf("  remove  %s? [ynaqh] ", qPrintable(file.fileName()));
            
            QString line = stream.readLine();
            if (line.isNull())
                break;
            
            if (line.isEmpty())
                continue;
            
            QCharRef c = line[0];
            if (c == 'Y' || c == 'y') {
                rmd << remove(file);
                break;
                
            } else if (c == 'N' || c == 'n') {
                break;
                
            } else if (c == 'A' || c == 'a') {
                allRemove = true;
                rmd << remove(file);
                break;

            } else if (c == 'Q' || c == 'q') {
                quit = true;
                break;
           
            } else if (c == 'H' || c == 'h') {
                printf("   y - yes, remove\n");
                printf("   n - no, do not remove\n");
                printf("   a - all, remove this and all others\n");
                printf("   q - quit, abort\n");
                printf("   h - help, show this help\n\n");
                
            } else {
                // one more
            }
        }
    }

    if (!proj.isEmpty()) {
        // Updates the project file
        ProjectFileGenerator(proj).remove(rmd);    
    }

    return rmd;
}


static QStringList rmfiles(const QStringList &files, const QString &proj)
{
    bool allRemove = false;
    bool quit = false;
    return rmfiles(files, allRemove, quit, proj);
}


static int random(int max)
{
    return (double)qrand() * (1.0 + max) / (1.0 + RAND_MAX);
}


static QByteArray randomString(int length)
{
    static char ch[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray ret;
    int max = strlen(ch) - 1;

    for (int i = 0; i < length; ++i) {
        ret += ch[random(max)];
    }
    return ret;
}


static bool createNewApplication(const QString &name)
{
    if (name.isEmpty()) {
         qCritical("invalid argument");
        return false;
    }

    QDir dir(".");
    if (dir.exists(name)) {
        qCritical("directory already exists");
        return false;
    }
    if (!dir.mkdir(name)) {
        qCritical("failed to create a directory %s", qPrintable(name));
        return false;
    }
    printf("  created   %s\n", qPrintable(name));

    // Creates sub-directories
    for (QStringListIterator i(*subDirs()); i.hasNext(); ) {
        const QString &str = i.next();
        QString d = name + SEP + str;
        if (!mkpath(dir, d)) {
            return false;
        }
    }

    // Copies files
    copy(dataDirPath + "app.pro", name + SEP + name + ".pro");

    for (QStringListIterator it(*filePaths()); it.hasNext(); ) {
        const QString &path = it.next();
        QString filename = QFileInfo(path).fileName();
        QString dst = name + SEP + path;
        copy(dataDirPath + filename, dst);

        // Replaces a string in application.ini file
        if (filename == "application.ini") {
            replaceString(dst, "$SessionSecret$", randomString(30));
        }
    }

    return true;
}


static int deleteScaffold(const QString &name)
{
    // Removes files
    QString str = name;
    str = str.remove('_').toLower().trimmed();
    if (str.endsWith("validator", Qt::CaseInsensitive)) {
        QStringList helpers;
        helpers << P_HELPERS + str + ".h"
                << P_HELPERS + str + ".cpp";
        
        rmfiles(helpers, P_HELPERS + "helpers.pro");
        
    } else {
        QStringList ctrls, models, views;
        ctrls << P_CTRLS + str + "controller.h"
              << P_CTRLS + str + "controller.cpp";
        
        models << P_MODELS + "sqlobjects" + SEP + str + "object.h"
               << P_MODELS + str + ".h"
               << P_MODELS + str + ".cpp";
        
        // Template system
        if (templateSystem == "otama") {
            views << P_VIEWS + str + SEP + "index.html"
                  << P_VIEWS + str + SEP + "index.otm"
                  << P_VIEWS + str + SEP + "show.html"
                  << P_VIEWS + str + SEP + "show.otm"
                  << P_VIEWS + str + SEP + "entry.html"
                  << P_VIEWS + str + SEP + "entry.otm"
                  << P_VIEWS + str + SEP + "edit.html"
                  << P_VIEWS + str + SEP + "edit.otm";
        } else if (templateSystem == "erb") {
            views << P_VIEWS + str + SEP + "index.erb"
                  << P_VIEWS + str + SEP + "show.erb"
                  << P_VIEWS + str + SEP + "entry.erb"
                  << P_VIEWS + str + SEP + "edit.erb";
        } else {
            qCritical("Invalid template system specified: %s", qPrintable(templateSystem));
            return 2;
        }
        
        bool allRemove = false;
        bool quit = false;
        
        // Removes controllers
        rmfiles(ctrls, allRemove, quit, P_CTRLS + "controllers.pro");
        if (quit) {
            ::_exit(1);
            return 1;
        }
        
        // Removes models
        rmfiles(models, allRemove, quit, P_MODELS + "models.pro");
        if (quit) {
            ::_exit(1);
            return 1;
        }
        
        // Removes views
        QStringList rmd = rmfiles(views, allRemove, quit);
        if (!rmd.isEmpty()) {
            QString path = P_VIEWS + "_src" + SEP + str;
            QFile::remove(path + "_indexView.cpp");
            QFile::remove(path + "_showView.cpp");
            QFile::remove(path + "_entryView.cpp");
            QFile::remove(path + "_editView.cpp");
        }
        
        // Removes the sub-directory
        rmpath(P_VIEWS + str);
    }
    return 0;
}


static bool checkIniFile()
{
    // Checking INI file
    if (!QFile::exists(appIni)) {
        qCritical("INI file not found, %s", qPrintable(appIni));
        qCritical("Execute %s command in application root directory!", qPrintable(QCoreApplication::arguments().value(0)));
        return false;
    }
    return true;
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qsrand(time(NULL));
    QStringList args = QCoreApplication::arguments();
    int subcmd = subCommands()->value(args.value(1), Invalid);

    switch (subcmd) {
    case Invalid:
        qCritical("invalid argument");
        return 1;
        break;

    case Help:
        usage();
        break;

    case New:
        // Creates new project
        if (!createNewApplication(args.value(2))) {
            return 1;
        }
        break;

    case ShowDrivers:
        printf("Available database drivers for Qt:\n");
        for (QStringListIterator i(TableSchema::databaseDrivers()); i.hasNext(); ) {
            printf("  %s\n", qPrintable(i.next()));
        }
        break;

    case ShowDriverPath: {
        QString path = QLibraryInfo::location(QLibraryInfo::PluginsPath) + QDir::separator() + "sqldrivers";
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isDir()) {
            qCritical("Error: database driver's directory not found");
            return 1;
        }
        printf("%s\n", qPrintable(fi.canonicalFilePath()));
        break; }

    case ShowTables:
        if (checkIniFile()) {
            QStringList tables = TableSchema::tables();
            if (!tables.isEmpty()) {
                printf("-----------------\nAvailable tables:\n");
                for (QStringListIterator i(tables); i.hasNext(); ) {
                    printf("  %s\n", qPrintable(i.next()));
                }
            }
        } else {
            return 2;
        }
        break;
        
    default: {
        if (argc < 3) {
            qCritical("invalid argument");
            return 1;
        }

        if (!checkIniFile()) {
            return 2;
        }

        // Sets codec
        QTextCodec *codec = QTextCodec::codecForName(appSettings.value("InternalEncoding").toByteArray().trimmed());
        codec = (codec) ? codec : QTextCodec::codecForLocale();
        QTextCodec::setCodecForLocale(codec);
#if QT_VERSION < 0x050000
        QTextCodec::setCodecForTr(codec);
        QTextCodec::setCodecForCStrings(codec);
#endif

        // ERB or Otama
        templateSystem = devSettings.value("TemplateSystem").toString().toLower();
        if (templateSystem.isEmpty()) {
            templateSystem = appSettings.value("TemplateSystem", "Erb").toString().toLower();
        }

        switch (subcmd) {
        case Controller: {
            QString ctrl = args.value(2);
            ControllerGenerator crtlgen(QString(), ctrl, args.mid(3), P_CTRLS);
            crtlgen.generate();
            
            // Create view directory
            QDir dir(P_VIEWS + ((ctrl.contains('_')) ? ctrl.toLower() : fieldNameToVariableName(ctrl).toLower()));
            mkpath(dir, ".");
            break; }
        
        case Model: {
            ModelGenerator modelgen(args.value(3), args.value(2), QStringList(), P_MODELS);
            modelgen.generate();
            break; }

        case UserModel: {
            ModelGenerator modelgen(args.value(5), args.value(2), args.mid(3, 2), P_MODELS);
            modelgen.generate(true);
            break; }
        
        case SqlObject: {
            ModelGenerator modelgen(args.value(3), args.value(2), QStringList(), P_MODELS);
            modelgen.generateSqlObject();
            break; }
       
        case Validator: {
            ValidatorGenerator validgen(args.value(2), P_HELPERS);
            validgen.generate();
            break; }

        case Mailer: {
            MailerGenerator mailgen(args.value(2), args.mid(3), P_CTRLS);
            mailgen.generate();
            copy(dataDirPath + "mail.erb", P_VIEWS + "mailer" + SEP +"mail.erb");
            break; }

        case Scaffold: {
            ControllerGenerator crtlgen(args.value(3), args.value(2), QStringList(), P_CTRLS);
            bool success = crtlgen.generate();
            
            ModelGenerator modelgen(args.value(3), args.value(2), QStringList(), P_MODELS);
            success &= modelgen.generate();
            
            // Generates view files of the specified template system
            if (templateSystem == "otama") {
                OtamaGenerator viewgen(args.value(3), args.value(2), P_VIEWS);
                viewgen.generate();
            } else if (templateSystem == "erb") {
                ErbGenerator viewgen(args.value(3), args.value(2), P_VIEWS);
                viewgen.generate();
            } else {
                qCritical("Invalid template system specified: %s", qPrintable(templateSystem));
                return 2;
            }

            if (success) {
                if (!QFile("Makefile").exists()) {
                    QProcess qmake;
                    QStringList args;
                    args << "-recursive";
#if defined(Q_OS_WIN32)
                    args << "-spec" << "win32-g++";
#elif defined(Q_OS_MAC)
                    args << "-spec" << "macx-g++";
#elif defined(Q_OS_LINUX)
                    args << "-spec" << "linux-g++";
#endif
                    args << "\"CONFIG+=debug\"";
                    qmake.start("qmake", args);
                    if (qmake.waitForStarted() && qmake.waitForFinished()) {
                        if (qmake.exitStatus() == QProcess::NormalExit)
                            printf("  created   Makefile\n");
                    }
                }

                putchar('\n');
                int port = appSettings.value("ListenPort").toInt();
                if (port > 0 && port <= USHRT_MAX)
                    printf(" Index page URL:  http://localhost:%d/%s/index\n\n", port, qPrintable(modelgen.model()));
            }
            break; }

        case Delete: {
            // Removes files
            int ret = deleteScaffold(args.value(2));
            if (ret)
                return ret;
            break; }
            
        default:
            qCritical("internal error");
            return 1;
        }
        break; }
    }
    return 0;
}
