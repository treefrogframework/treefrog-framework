/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "controllergenerator.h"
#include "erbgenerator.h"
#include "global.h"
#include "helpergenerator.h"
#include "mailergenerator.h"
#include "servicegenerator.h"
#include "modelgenerator.h"
#include "mongocommand.h"
#include "mongoobjgenerator.h"
#include "otamagenerator.h"
#include "vueservicegenerator.h"
#include "vueerbgenerator.h"
#include "projectfilegenerator.h"
#include "sqlobjgenerator.h"
#include "tableschema.h"
#include "util.h"
#include "validatorgenerator.h"
#include "websocketgenerator.h"
#include "apicontrollergenerator.h"
#include "apiservicegenerator.h"
#include <QtCore>
#include <QTextCodec>
#include <random>
#ifndef Q_CC_MSVC
#include <unistd.h>
#endif
#include <ctime>

#define L(str) QLatin1String(str)
#define D_CTRLS QLatin1String("controllers/")
#define D_MODELS QLatin1String("models/")
#define D_VIEWS QLatin1String("views/")
#define D_HELPERS QLatin1String("helpers/")

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
    Api,
    Validator,
    Mailer,
    Scaffold,
    Delete,
    ShowDrivers,
    ShowDriverPath,
    ShowTables,
    ShowCollections,
};

class SubCommands : public QHash<QString, int> {
public:
    SubCommands() :
        QHash<QString, int>()
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
        insert("api", Api);
        insert("a", Api);
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


class SubDirs : public QStringList {
public:
    SubDirs() :
        QStringList()
    {
        append(L("controllers"));
        append(L("models"));
        append(L("models/objects"));
        append(L("models/sqlobjects"));
        append(L("models/mongoobjects"));
        append(L("views"));
        append(L("views/layouts"));
        append(L("views/mailer"));
        append(L("views/partial"));
        append(L("views/_src"));
        append(L("helpers"));
        append(L("config"));
        append(L("public"));
        append(L("public/images"));
        append(L("public/js"));
        append(L("public/css"));
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


class FilePaths : public QStringList {
public:
    FilePaths() :
        QStringList()
    {
        append(L("appbase.pri"));
        append(L("controllers/controllers.pro"));
        append(L("controllers/applicationcontroller.h"));
        append(L("controllers/applicationcontroller.cpp"));
        append(L("models/models.pro"));
#ifdef Q_OS_WIN
        append(L("models/objects/_dummymodel.h"));
        append(L("models/objects/_dummymodel.cpp"));
#endif
        append(L("views/views.pro"));
        append(L("views/_src/_src.pro"));
        append(L("views/layouts/.trim_mode"));
        append(L("views/mailer/.trim_mode"));
        append(L("views/partial/.trim_mode"));
        append(L("helpers/helpers.pro"));
        append(L("helpers/applicationhelper.h"));
        append(L("helpers/applicationhelper.cpp"));
        append(L("config/application.ini"));
        append(L("config/database.ini"));
        append(L("config/development.ini"));
        append(L("config/internet_media_types.ini"));
        append(L("config/logger.ini"));
        append(L("config/mongodb.ini"));
        append(L("config/redis.ini"));
        append(L("config/routes.cfg"));
        append(L("config/validation.ini"));
        append(L("config/cache.ini"));
        append(L("public/403.html"));
        append(L("public/404.html"));
        append(L("public/413.html"));
        append(L("public/500.html"));
        append(L("script/JSXTransformer.js"));
        append(L("script/react.js"));  // React
        append(L("script/react-with-addons.js"));  // React
        append(L("script/react-dom-server.js"));  // React
        // CMake
        append(L("CMakeLists.txt"));
        append(L("cmake/CacheClean.cmake"));
        append(L("cmake/TargetCmake.cmake"));
        append(L("controllers/CMakeLists.txt"));
        append(L("models/CMakeLists.txt"));
        append(L("views/CMakeLists.txt"));
        append(L("helpers/CMakeLists.txt"));
    }
};
Q_GLOBAL_STATIC(FilePaths, filePaths)


const QString appIni = QLatin1String("config/application.ini");
const QString devIni = QLatin1String("config/development.ini");
static QSettings appSettings(appIni, QSettings::IniFormat);
static QSettings devSettings(devIni, QSettings::IniFormat);
static QString templateSystem;

static void usage()
{
    std::printf("usage: tspawn <subcommand> [args]\n\n"
                "Type 'tspawn --show-drivers' to show all the available database drivers for Qt.\n"
                "Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.\n"
                "Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.\n"
                "Type 'tspawn --show-collections' to show all collections in the MongoDB.\n\n"
                "Available subcommands:\n"
                "  new (n)         <application-name>\n"
                "  scaffold (s)    <table-name> [model-name]\n"
                "  controller (c)  <controller-name> action [action ...]\n"
                "  model (m)       <table-name> [model-name]\n"
                "  helper (h)      <name>\n"
                "  usermodel (u)   <table-name> [username password [model-name]]\n"
                "  sqlobject (o)   <table-name> [model-name]\n"
                "  mongoscaffold (ms) <model-name>\n"
                "  mongomodel (mm) <model-name>\n"
                "  websocket (w)   <endpoint-name>\n"
                "  api (a)         <api-name>\n"
                "  validator (v)   <name>\n"
                "  mailer (l)      <mailer-name> action [action ...]\n"
                "  delete (d)      <table-name, helper-name or validator-name>\n");
}


static QStringList rmfiles(const QStringList &files, bool &allRemove, bool &quit, const QString &baseDir, const QString &proj = QString())
{
    QStringList rmd;

    // Removes files
    for (QStringListIterator i(files); i.hasNext();) {
        if (quit)
            break;

        const QString &fname = i.next();
        QFile file(baseDir + "/" + fname);
        if (!file.exists())
            continue;

        if (allRemove) {
            remove(file);
            rmd << fname;
            continue;
        }

        QTextStream stream(stdin);
        for (;;) {
            std::printf("  remove  %s? [ynaqh] ", qUtf8Printable(QDir::cleanPath(file.fileName())));

            QString line = stream.readLine().trimmed();
            if (line.isNull())
                break;

            if (line.isEmpty())
                continue;

            const QChar c = line[0];
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
                std::printf("   y - yes, remove\n");
                std::printf("   n - no, do not remove\n");
                std::printf("   a - all, remove this and all others\n");
                std::printf("   q - quit, abort\n");
                std::printf("   h - help, show this help\n\n");

            } else {
                // one more
            }
        }
    }

    if (!proj.isEmpty()) {
        // Updates the project file
        ProjectFileGenerator(baseDir + "/" + proj).remove(rmd);
    }

    return rmd;
}


static QStringList rmfiles(const QStringList &files, const QString &baseDir, const QString &proj)
{
    bool allRemove = false;
    bool quit = false;
    return rmfiles(files, allRemove, quit, baseDir, proj);
}


static uint random(uint max)
{
    static std::random_device randev;
    static std::default_random_engine eng(randev());
    std::uniform_int_distribution<uint64_t> uniform(0, max);
    return uniform(eng);
}


static QByteArray randomString(int length)
{
    constexpr auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
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
        qCritical("failed to create a directory %s", qUtf8Printable(name));
        return false;
    }
    std::printf("  created   %s\n", qUtf8Printable(name));

    // Creates sub-directories
    for (const QString &str : *subDirs()) {
        QString d = name + "/" + str;
        if (!mkpath(dir, d)) {
            return false;
        }
    }

    // Copies files
    copy(dataDirPath + "app.pro", name + "/" + name + ".pro");

    for (auto &path : *filePaths()) {
        QString filename = QFileInfo(path).fileName();
        QString dst = name + "/" + path;

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
    ProjectFileGenerator progen(name + "/" + D_MODELS + "models.pro");
    QStringList dummy = {"objects/_dummymodel.h", "objects/_dummymodel.cpp"};
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
              << str + "controller.cpp"
              << "api" + str + "controller.h"
              << "api" + str + "controller.cpp";

        models << QLatin1String("sqlobjects/") + str + "object.h"
               << QLatin1String("mongoobjects/") + str + "object.h"
               << QLatin1String("objects/") + str + ".h"
               << QLatin1String("objects/") + str + ".cpp"
               << str + "service.h"
               << str + "service.cpp"
               << "api" + str + "service.h"
               << "api" + str + "service.cpp";

        // Template system
        if (templateSystem == "otama") {
            views << str + "/index.html"
                  << str + "/index.otm"
                  << str + "/show.html"
                  << str + "/show.otm"
                  << str + "/create.html"
                  << str + "/create.otm"
                  << str + "/save.html"
                  << str + "/save.otm";
        } else if (templateSystem == "erb") {
            views << str + "/index.erb"
                  << str + "/show.erb"
                  << str + "/create.erb"
                  << str + "/save.erb";
        } else {
            qCritical("Invalid template system specified: %s", qUtf8Printable(templateSystem));
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
            QString path = D_VIEWS + "_src/" + str;
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
        qCritical("INI file not found, %s", qUtf8Printable(appIni));
        qCritical("Execute %s command in application root directory!", qUtf8Printable(QCoreApplication::arguments().value(0)));
        return false;
    }
    return true;
}


static void printSuccessMessage(const QString &model)
{
    QString msg;
    if (!QFile("Makefile").exists() && !QFile(L("build/Makefile")).exists()) {
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
        std::printf(" Index page URL:  http://localhost:%d/%s/index\n\n", port, qUtf8Printable(model));

    if (!msg.isEmpty()) {
        puts(qUtf8Printable(msg));
    }
}


static bool isVueEnabled()
{
    static int vueEnable = -1;

    if (vueEnable < 0) {
        std::printf("\n");
        QTextStream stream(stdin);
        for (;;) {
            std::printf(" Create sources for vue.js? [y/n] ");
            QString line = stream.readLine().trimmed();

            const QChar c = line[0];
            if (c == 'Y' || c == 'y') {
                vueEnable = 1;
                break;
            } else if (c == 'N' || c == 'n') {
                vueEnable = 0;
                break;
            }
        }
    }
    return (bool)vueEnable;
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
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
        std::printf("Available database drivers for Qt:\n");
        for (QStringListIterator i(TableSchema::databaseDrivers()); i.hasNext();) {
            std::printf("  %s\n", qUtf8Printable(i.next()));
        }
        break;

    case ShowDriverPath: {
#if QT_VERSION < 0x060000
        QString path = QLibraryInfo::location(QLibraryInfo::PluginsPath) + "/sqldrivers";
#else
        QString path = QLibraryInfo::path(QLibraryInfo::PluginsPath) + "/sqldrivers";
#endif
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isDir()) {
            qCritical("Error: database driver's directory not found");
            return 1;
        }
        std::printf("%s\n", qUtf8Printable(fi.canonicalFilePath()));
        break;
    }

    case ShowTables:
        if (checkIniFile()) {
            QStringList tables = TableSchema::tables();
            if (!tables.isEmpty()) {
                std::printf("-----------------\nAvailable tables:\n");
                for (QStringListIterator i(tables); i.hasNext();) {
                    std::printf("  %s\n", qUtf8Printable(i.next()));
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
            QString mnginipath = QLatin1String("config/") + mongoini;

            if (mongoini.isEmpty() || !QFile(mnginipath).exists()) {
                qCritical("MongoDB settings file not found");
                return 2;
            }

            MongoCommand mongo(mnginipath);
            if (!mongo.open("dev")) {
                return 2;
            }

            QStringList colls = mongo.getCollectionNames();
            std::printf("-----------------\nExisting collections:\n");
            for (auto &col : colls) {
                std::printf("  %s\n", qUtf8Printable(col));
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
            break;
        }

        case Model: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            if (isVueEnabled()) {
                VueServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            } else {
                ServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            }
            break;
        }

        case Helper: {
            HelperGenerator helpergen(args.value(2));
            helpergen.generate(D_HELPERS);
            break;
        }

        case UserModel: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(5), args.value(2), args.mid(3, 2));
            modelgen.generate(D_MODELS, true);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            if (isVueEnabled()) {
                VueServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            } else {
                ServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            }
            break;
        }

        case SqlObject: {
            SqlObjGenerator sqlgen(args.value(3), args.value(2));
            QString path = sqlgen.generate(D_MODELS);

            // Generates a project file
            ProjectFileGenerator progen(D_MODELS + "models.pro");
            progen.add(QStringList(path));
            break;
        }

        case MongoScaffold: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            bool success = modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            if (isVueEnabled()) {
                VueServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                success &= svrgen.generate(D_MODELS);
            } else {
                ServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                success &= svrgen.generate(D_MODELS);
            }

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
                qCritical("Invalid template system specified: %s", qUtf8Printable(templateSystem));
                return 2;
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break;
        }

        case MongoModel: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            if (isVueEnabled()) {
                VueServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            } else {
                ServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);
            }
            break;
        }

        case WebSocketEndpoint: {
            const QString appendpointfiles[] = {L("controllers/applicationendpoint.h"),
                L("controllers/applicationendpoint.cpp")};

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
            break;
        }

        case Api: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            ApiServiceGenerator srvgen(modelgen.model(), modelgen.fieldList(), pkidx);
            srvgen.generate(D_MODELS);

            ApiControllerGenerator apigen(modelgen.model(), modelgen.fieldList(), pkidx);
            apigen.generate(D_CTRLS);
            break;
        }

        case Validator: {
            ValidatorGenerator validgen(args.value(2));
            validgen.generate(D_HELPERS);
            break;
        }

        case Mailer: {
            MailerGenerator mailgen(args.value(2), args.mid(3));
            mailgen.generate(D_CTRLS);
            copy(dataDirPath + "mail.erb", D_VIEWS + "mailer/mail.erb");
            break;
        }

        case Scaffold: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            bool success = modelgen.generate(D_MODELS);

            if (!success) {
                return 2;
            }

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            ControllerGenerator crtlgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            success &= crtlgen.generate(D_CTRLS);

            if (isVueEnabled()) {
                VueServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);

                // Generates view files of the specified template system
                if (templateSystem == "erb") {
                    VueErbGenerator viewgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.autoValueIndex());
                    viewgen.generate(D_VIEWS);
                } else {
                    qCritical("Invalid template system specified: %s", qUtf8Printable(templateSystem));
                    return 2;
                }
            } else {
                ServiceGenerator svrgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
                svrgen.generate(D_MODELS);

                // Generates view files of the specified template system
                if (templateSystem == "otama") {
                    OtamaGenerator viewgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.autoValueIndex());
                    viewgen.generate(D_VIEWS);
                } else if (templateSystem == "erb") {
                    ErbGenerator viewgen(modelgen.model(), modelgen.fieldList(), pkidx, modelgen.autoValueIndex());
                    viewgen.generate(D_VIEWS);
                } else {
                    qCritical("Invalid template system specified: %s", qUtf8Printable(templateSystem));
                    return 2;
                }
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break;
        }

        case Delete: {
            // Removes files
            int ret = deleteScaffold(args.value(2));
            if (ret) {
                return ret;
            }
            break;
        }

        default:
            qCritical("internal error");
            return 1;
        }
        break;
    }
    }
    return 0;
}
