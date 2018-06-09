/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include "global.h"
#include "controllergenerator.h"
#include "modelgenerator.h"
#include "helpergenerator.h"
#include "sqlobjgenerator.h"
#include "mongoobjgenerator.h"
#include "websocketgenerator.h"
#include "validatorgenerator.h"
#include "otamagenerator.h"
#include "erbgenerator.h"
#include "mailergenerator.h"
#include "projectfilegenerator.h"
#include "tableschema.h"
#include "mongocommand.h"
#include "util.h"
#ifndef Q_CC_MSVC
# include <unistd.h>
#endif
#include <ctime>

#define L(str)  QLatin1String(str)
#define SEP   QDir::separator()
#define D_CTRLS   (QLatin1String("controllers") + SEP)
#define D_MODELS  (QLatin1String("models") + SEP)
#define D_VIEWS   (QLatin1String("views") + SEP)
#define D_HELPERS (QLatin1String("helpers") + SEP)

enum SubCommand {
    Invalid = 0,
    Help,
    New,
    Controller,
    Model,
    Helper,
    UserModel,
    SqlObject,
    //UpdateModel,
    MongoScaffold,
    MongoModel,
    WebSocketEndpoint,
    Validator,
    Mailer,
    Scaffold,
    Delete,
    ShowDrivers,
    ShowDriverPath,
    ShowTables,
    ShowCollections,
};

class SubCommands : public QHash<QString, int>
{
public:
    SubCommands() : QHash<QString, int>()
    {
        insert("-h", Help);
        insert("--help", Help);
        insert("new", New);
        insert("n", New);
        insert("controller", Controller);
        insert("c", Controller);
        insert("model", Model);
        insert("m", Model);
        insert("helper", Helper);
        insert("h", Helper);
        insert("usermodel", UserModel);
        insert("u", UserModel);
        insert("sqlobject", SqlObject);
        insert("o", SqlObject);
        insert("mongoscaffold", MongoScaffold);
        insert("ms", MongoScaffold);
        // insert("updatemodel", UpdateModel);
        // insert("um", UpdateModel);
        insert("mongomodel", MongoModel);
        insert("mm", MongoModel);
        insert("websocket", WebSocketEndpoint);
        insert("w", WebSocketEndpoint);
        insert("validator", Validator);
        insert("v", Validator);
        insert("mailer", Mailer);
        insert("l", Mailer);
        insert("scaffold", Scaffold);
        insert("s", Scaffold);
        insert("delete", Delete);
        insert("d", Delete);
        insert("remove", Delete);
        insert("r", Delete);
        insert("--show-drivers", ShowDrivers);
        insert("--show-driver-path", ShowDriverPath);
        insert("--show-tables", ShowTables);
        insert("--show-collections", ShowCollections);
    }
};
Q_GLOBAL_STATIC(SubCommands, subCommands)


class SubDirs : public QStringList
{
public:
    SubDirs() : QStringList()
    {
        append(L("controllers"));
        append(L("models"));
        append(L("models") + SEP + "sqlobjects");
        append(L("models") + SEP + "mongoobjects");
        append(L("views"));
        append(L("views") + SEP + "layouts");
        append(L("views") + SEP + "mailer");
        append(L("views") + SEP + "partial");
        append(L("views") + SEP + "_src");
        append(L("helpers"));
        append(L("config"));
        append(L("public"));
        append(L("public") + SEP + "images");
        append(L("public") + SEP + "js");
        append(L("public") + SEP + "css");
        append(L("db"));
        append(L("lib"));
        append(L("log"));
        append(L("plugin"));
        append(L("script"));
        append(L("sql"));
        append(L("test"));
        append(L("tmp"));
        append(L("cmake"));
    }
};
Q_GLOBAL_STATIC(SubDirs, subDirs)


class FilePaths : public QStringList
{
public:
    FilePaths() : QStringList()
    {
        append(L("appbase.pri"));
        append(L("controllers") + SEP + "controllers.pro");
        append(L("controllers") + SEP + "applicationcontroller.h");
        append(L("controllers") + SEP + "applicationcontroller.cpp");
        append(L("models") + SEP + "models.pro");
#ifdef Q_OS_WIN
        append(L("models") + SEP + "_dummymodel.h");
        append(L("models") + SEP + "_dummymodel.cpp");
#endif
        append(L("views") + SEP + "views.pro");
        append(L("views") + SEP + "_src" + SEP + "_src.pro");
        append(L("views") + SEP + "layouts" + SEP + ".trim_mode");
        append(L("views") + SEP + "mailer"  + SEP + ".trim_mode");
        append(L("views") + SEP + "partial" + SEP + ".trim_mode");
        append(L("helpers") + SEP + "helpers.pro");
        append(L("helpers") + SEP + "applicationhelper.h");
        append(L("helpers") + SEP + "applicationhelper.cpp");
        append(L("config") + SEP + "application.ini");
        append(L("config") + SEP + "database.ini");
        append(L("config") + SEP + "development.ini");
        append(L("config") + SEP + "internet_media_types.ini");
        append(L("config") + SEP + "logger.ini");
        append(L("config") + SEP + "mongodb.ini");
        append(L("config") + SEP + "redis.ini");
        append(L("config") + SEP + "routes.cfg");
        append(L("config") + SEP + "validation.ini");
        append(L("public") + SEP + "403.html");
        append(L("public") + SEP + "404.html");
        append(L("public") + SEP + "413.html");
        append(L("public") + SEP + "500.html");
        append(L("script") + SEP + "JSXTransformer.js");
        append(L("script") + SEP + "react.js");             // React
        append(L("script") + SEP + "react-with-addons.js"); // React
        append(L("script") + SEP + "react-dom-server.js");  // React
        // CMake
        append(L("CMakeLists.txt"));
        append(L("cmake") + SEP +"CacheClean.cmake");
        append(L("cmake") + SEP +"TargetCmake.cmake");
        append(L("controllers") + SEP + "CMakeLists.txt");
        append(L("models") + SEP + "CMakeLists.txt");
        append(L("views") + SEP + "CMakeLists.txt");
        append(L("helpers") + SEP + "CMakeLists.txt");
    }
};
Q_GLOBAL_STATIC(FilePaths, filePaths)


const QString appIni = QLatin1String("config") + SEP + "application.ini";
const QString devIni = QLatin1String("config") + SEP + "development.ini";
static QSettings appSettings(appIni, QSettings::IniFormat);
static QSettings devSettings(devIni, QSettings::IniFormat);
static QString templateSystem;


static void usage()
{
    printf("usage: tspawn <subcommand> [args]\n\n" \
           "Type 'tspawn --show-drivers' to show all the available database drivers for Qt.\n" \
           "Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.\n" \
           "Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.\n" \
           "Type 'tspawn --show-collections' to show all collections in the MongoDB.\n\n" \
           "Available subcommands:\n" \
           "  new (n)         <application-name>\n" \
           "  scaffold (s)    <table-name> [model-name]\n" \
           "  controller (c)  <controller-name> action [action ...]\n" \
           "  model (m)       <table-name> [model-name]\n" \
           "  helper (h)      <name>\n" \
           "  usermodel (u)   <table-name> [username password [model-name]]\n" \
           "  sqlobject (o)   <table-name> [model-name]\n"         \
           "  mongoscaffold (ms) <model-name>\n"                   \
           "  mongomodel (mm) <model-name>\n"                      \
           "  websocket (w)   <endpoint-name>\n"                   \
           "  validator (v)   <name>\n"                            \
           "  mailer (l)      <mailer-name> action [action ...]\n" \
           "  delete (d)      <table-name, helper-name or validator-name>\n");
}


static QStringList rmfiles(const QStringList &files, bool &allRemove, bool &quit, const QString &baseDir, const QString &proj = QString())
{
    QStringList rmd;

    // Removes files
    for (QStringListIterator i(files); i.hasNext(); ) {
        if (quit)
            break;

        const QString &fname = i.next();
        QFile file(baseDir + SEP + fname);
        if (!file.exists())
            continue;

        if (allRemove) {
            remove(file);
            rmd << fname;
            continue;
        }

        QTextStream stream(stdin);
        for (;;) {
            printf("  remove  %s? [ynaqh] ", qPrintable(QDir::cleanPath(file.fileName())));

            QString line = stream.readLine();
            if (line.isNull())
                break;

            if (line.isEmpty())
                continue;

            QCharRef c = line[0];
            if (c == 'Y' || c == 'y') {
                remove(file);
                rmd << fname;
                break;

            } else if (c == 'N' || c == 'n') {
                break;

            } else if (c == 'A' || c == 'a') {
                allRemove = true;
                remove(file);
                rmd << fname;
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
        ProjectFileGenerator(baseDir + SEP + proj).remove(rmd);
    }

    return rmd;
}


static QStringList rmfiles(const QStringList &files, const QString &baseDir, const QString &proj)
{
    bool allRemove = false;
    bool quit = false;
    return rmfiles(files, allRemove, quit, baseDir, proj);
}


static int random(int max)
{
    return (int)((double)qrand() * (1.0 + max) / (1.0 + RAND_MAX));
}


static QByteArray randomString(int length)
{
    static char ch[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray ret;
    int max = (int)strlen(ch) - 1;

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
    for (const QString &str : *subDirs()) {
        QString d = name + SEP + str;
        if (!mkpath(dir, d)) {
            return false;
        }
    }

    // Copies files
    copy(dataDirPath + "app.pro", name + SEP + name + ".pro");

    for (auto &path : *filePaths()) {
        QString filename = QFileInfo(path).fileName();
        QString dst = name + SEP + path;

        if (filename == "CMakeLists.txt") {
            copy(dataDirPath + path, dst);
        } else {
            copy(dataDirPath + filename, dst);
        }

        // Replaces a string in application.ini file
        if (filename == "application.ini") {
            replaceString(dst, "$SessionSecret$", randomString(30));
        }
    }

#ifdef Q_OS_WIN
    // Add dummy model files
    ProjectFileGenerator progen(name + SEP + D_MODELS + "models.pro");
    QStringList dummy = { "_dummymodel.h", "_dummymodel.cpp" };
    progen.add(dummy);
#endif

    return true;
}


static int deleteScaffold(const QString &name)
{
    // Removes files
    QString str = name;
    str = str.remove('_').toLower().trimmed();

    if (QFileInfo(D_HELPERS + str + ".h").exists()) {
        QStringList helpers;
        helpers << str + ".h"
                << str + ".cpp";

        rmfiles(helpers, D_HELPERS, "helpers.pro");

    } else if (str.endsWith("validator", Qt::CaseInsensitive)) {
        QStringList helpers;
        helpers << str + ".h"
                << str + ".cpp";

        rmfiles(helpers, D_HELPERS, "helpers.pro");

    } else {
        QStringList ctrls, models, views;
        ctrls << str + "controller.h"
              << str + "controller.cpp";

        models << QLatin1String("sqlobjects") + SEP + str + "object.h"
               << QLatin1String("mongoobjects") + SEP + str + "object.h"
               << str + ".h"
               << str + ".cpp";

        // Template system
        if (templateSystem == "otama") {
            views << str + SEP + "index.html"
                  << str + SEP + "index.otm"
                  << str + SEP + "show.html"
                  << str + SEP + "show.otm"
                  << str + SEP + "create.html"
                  << str + SEP + "create.otm"
                  << str + SEP + "save.html"
                  << str + SEP + "save.otm";
        } else if (templateSystem == "erb") {
            views << str + SEP + "index.erb"
                  << str + SEP + "show.erb"
                  << str + SEP + "create.erb"
                  << str + SEP + "save.erb";
        } else {
            qCritical("Invalid template system specified: %s", qPrintable(templateSystem));
            return 2;
        }

        bool allRemove = false;
        bool quit = false;

        // Removes controllers
        rmfiles(ctrls, allRemove, quit, D_CTRLS, "controllers.pro");
        if (quit) {
            ::_exit(1);
            return 1;
        }

        // Removes models
        rmfiles(models, allRemove, quit, D_MODELS, "models.pro");
        if (quit) {
            ::_exit(1);
            return 1;
        }

        // Removes views
        QStringList rmd = rmfiles(views, allRemove, quit, D_VIEWS);
        if (!rmd.isEmpty()) {
            QString path = D_VIEWS + "_src" + SEP + str;
            QFile::remove(path + "_indexView.cpp");
            QFile::remove(path + "_showView.cpp");
            QFile::remove(path + "_entryView.cpp");
            QFile::remove(path + "_editView.cpp");
        }

        // Removes the sub-directory
        rmpath(D_VIEWS + str);
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


static void printSuccessMessage(const QString &model)
{
    QString msg;
    if (!QFile("Makefile").exists() && !QFile(L("build") + SEP + "Makefile").exists()) {
        msg = "qmake:\n Run `qmake -r%0 CONFIG+=debug` to generate a Makefile for debug mode.\n Run `qmake -r%0 CONFIG+=release` to generate a Makefile for release mode.\n";
#ifdef Q_OS_DARWIN
        msg = msg.arg(" -spec macx-clang");
#else
        msg = msg.arg("");
#endif
        msg += "\n or\n\ncmake:\n";
        msg += " Run `mkdir build; cd build; cmake ..` to generate a Makefile.\n";
        msg += " Run `cd build; make cmake` to regenerate the Makefile.";
    }

    putchar('\n');
    int port = appSettings.value("ListenPort").toInt();
    if (port > 0 && port <= USHRT_MAX)
        printf(" Index page URL:  http://localhost:%d/%s/index\n\n", port, qPrintable(model));

    if (!msg.isEmpty()) {
        puts(qPrintable(msg));
    }
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
        QString path = QLibraryInfo::location(QLibraryInfo::PluginsPath) + SEP + "sqldrivers";
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
                putchar('\n');
            }
        } else {
            return 2;
        }
        break;

    case ShowCollections:
        if (checkIniFile()) {
            // MongoDB settings
            QString mongoini = appSettings.value("MongoDbSettingsFile").toString().trimmed();
            QString mnginipath = QLatin1String("config") + SEP + mongoini;

            if (mongoini.isEmpty() || !QFile(mnginipath).exists()) {
                qCritical("MongoDB settings file not found");
                return 2;
            }

            MongoCommand mongo(mnginipath);
            if (!mongo.open("dev")) {
                return 2;
            }

            QStringList colls = mongo.getCollectionNames();
            printf("-----------------\nExisting collections:\n");
            for (auto &col : colls) {
                printf("  %s\n", qPrintable(col));
            }
            putchar('\n');
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

        // ERB or Otama
        templateSystem = devSettings.value("TemplateSystem").toString().toLower();
        if (templateSystem.isEmpty()) {
            templateSystem = appSettings.value("TemplateSystem", "Erb").toString().toLower();
        }

        switch (subcmd) {
        case Controller: {
            QString ctrl = args.value(2);
            ControllerGenerator crtlgen(ctrl, args.mid(3));
            crtlgen.generate(D_CTRLS);

            // Create view directory
            QDir dir(D_VIEWS + ((ctrl.contains('_')) ? ctrl.toLower() : fieldNameToVariableName(ctrl).toLower()));
            mkpath(dir, ".");
            copy(dataDirPath + ".trim_mode", dir);
            break; }

        case Model: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            modelgen.generate(D_MODELS);
            break; }

        case Helper: {
            HelperGenerator helpergen(args.value(2));
            helpergen.generate(D_HELPERS);
            break; }

        case UserModel: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(5), args.value(2), args.mid(3, 2));
            modelgen.generate(D_MODELS, true);
            break; }

        case SqlObject: {
            SqlObjGenerator sqlgen(args.value(3), args.value(2));
            QString path = sqlgen.generate(D_MODELS);

            // Generates a project file
            ProjectFileGenerator progen(D_MODELS + "models.pro");
            progen.add(QStringList(path));
            break; }

        case MongoScaffold: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            bool success = modelgen.generate(D_MODELS);

            ControllerGenerator crtlgen(modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.lockRevisionIndex());
            success &= crtlgen.generate(D_CTRLS);

            // Generates view files of the specified template system
            if (templateSystem == "otama") {
                OtamaGenerator viewgen(modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.autoValueIndex());
                viewgen.generate(D_VIEWS);
            } else if (templateSystem == "erb") {
                ErbGenerator viewgen(modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.autoValueIndex());
                viewgen.generate(D_VIEWS);
            } else {
                qCritical("Invalid template system specified: %s", qPrintable(templateSystem));
                return 2;
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break; }

        case MongoModel: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            modelgen.generate(D_MODELS);
            break; }

        case WebSocketEndpoint: {
            const QString appendpointfiles[] = { L("controllers") + SEP + "applicationendpoint.h",
                                                 L("controllers") + SEP + "applicationendpoint.cpp" };

            ProjectFileGenerator progen(D_CTRLS + "controllers.pro");

            for (auto &dst : appendpointfiles) {
                if (!QFile::exists(dst)) {
                    QString filename = QFileInfo(dst).fileName();
                    copy(dataDirPath + filename, dst);
                    progen.add(QStringList(filename));
                }
            }

            WebSocketGenerator wsgen(args.value(2));
            wsgen.generate(D_CTRLS);
            break; }

        case Validator: {
            ValidatorGenerator validgen(args.value(2));
            validgen.generate(D_HELPERS);
            break; }

        case Mailer: {
            MailerGenerator mailgen(args.value(2), args.mid(3));
            mailgen.generate(D_CTRLS);
            copy(dataDirPath + "mail.erb", D_VIEWS + "mailer" + SEP +"mail.erb");
            break; }

        case Scaffold: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            bool success = modelgen.generate(D_MODELS);

            if (!success)
                return 2;

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qPrintable(args.value(2)));
                return 2;
            }

            ControllerGenerator crtlgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            success &= crtlgen.generate(D_CTRLS);

            // Generates view files of the specified template system
            if (templateSystem == "otama") {
                OtamaGenerator viewgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.autoValueIndex());
                viewgen.generate(D_VIEWS);
            } else if (templateSystem == "erb") {
                ErbGenerator viewgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.autoValueIndex());
                viewgen.generate(D_VIEWS);
            } else {
                qCritical("Invalid template system specified: %s", qPrintable(templateSystem));
                return 2;
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break; }

        case Delete: {
            // Removes files
            int ret = deleteScaffold(args.value(2));
            if (ret) {
                return ret;
            }
            break; }

        default:
            qCritical("internal error");
            return 1;
        }
        break; }
    }
    return 0;
}
