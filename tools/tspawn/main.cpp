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
#include "vitevuegenerator.h"
#include "vitevueservicegenerator.h"
#include "projectfilegenerator.h"
#include "sqlobjgenerator.h"
#include "tableschema.h"
#include "util.h"
#include "validatorgenerator.h"
#include "websocketgenerator.h"
#include "apicontrollergenerator.h"
#include "apiservicegenerator.h"
#include <QtCore>
#include <memory>
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

enum class SubCommand {
    Invalid,
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

enum class TemplateSystem {
    Invalid,
    Erb,
    Otama,
    Vue,
    Vite_Vue,
};

const QMap<QString, SubCommand> subCommands = {
    {"-h", SubCommand::Help},
    {"--help", SubCommand::Help},
    {"new", SubCommand::New},
    {"n", SubCommand::New},
    {"controller", SubCommand::Controller},
    {"c", SubCommand::Controller},
    {"model", SubCommand::Model},
    {"m", SubCommand::Model},
    {"helper", SubCommand::Helper},
    {"h", SubCommand::Helper},
    {"usermodel", SubCommand::UserModel},
    {"u", SubCommand::UserModel},
    {"sqlobject", SubCommand::SqlObject},
    {"o", SubCommand::SqlObject},
    {"mongoscaffold", SubCommand::MongoScaffold},
    {"ms", SubCommand::MongoScaffold},
    //{"updatemodel", SubCommand::UpdateModel},
    //{"um", SubCommand::UpdateModel},
    {"mongomodel", SubCommand::MongoModel},
    {"mm", SubCommand::MongoModel},
    {"websocket", SubCommand::WebSocketEndpoint},
    {"w", SubCommand::WebSocketEndpoint},
    {"api", SubCommand::Api},
    {"a", SubCommand::Api},
    {"validator", SubCommand::Validator},
    {"v", SubCommand::Validator},
    {"mailer", SubCommand::Mailer},
    {"l", SubCommand::Mailer},
    {"scaffold", SubCommand::Scaffold},
    {"s", SubCommand::Scaffold},
    {"delete", SubCommand::Delete},
    {"d", SubCommand::Delete},
    {"remove", SubCommand::Delete},
    {"r", SubCommand::Delete},
    {"--show-drivers", SubCommand::ShowDrivers},
    {"--show-driver-path", SubCommand::ShowDriverPath},
    {"--show-tables", SubCommand::ShowTables},
    {"--show-collections", SubCommand::ShowCollections},
};

const QMap<QString, TemplateSystem> templateSystemMap = {
    {"erb", TemplateSystem::Erb},
    {"otama", TemplateSystem::Otama},
    {"vue", TemplateSystem::Vue},
    {"vite+vue", TemplateSystem::Vite_Vue},
};

const QStringList subDirs = {
    L("controllers"),
    L("models"),
    L("models/objects"),
    L("models/sqlobjects"),
    L("models/mongoobjects"),
    L("views"),
    L("views/layouts"),
    L("views/mailer"),
    L("views/partial"),
    L("views/_src"),
    L("helpers"),
    L("config"),
    L("public"),
    L("public/images"),
    L("public/js"),
    L("public/css"),
    L("db"),
    L("lib"),
    L("log"),
    L("plugin"),
    L("script"),
    L("sql"),
    L("test"),
    L("tmp"),
    L("cmake"),
};

const QStringList filePaths = {
    L("appbase.pri"),
    L("controllers/controllers.pro"),
    L("controllers/applicationcontroller.h"),
    L("controllers/applicationcontroller.cpp"),
    L("models/models.pro"),
#ifdef Q_OS_WIN
    L("models/objects/_dummymodel.h"),
    L("models/objects/_dummymodel.cpp"),
#endif
    L("views/views.pro"),
    L("views/_src/_src.pro"),
    L("views/layouts/.trim_mode"),
    L("views/mailer/.trim_mode"),
    L("views/partial/.trim_mode"),
    L("helpers/helpers.pro"),
    L("helpers/applicationhelper.h"),
    L("helpers/applicationhelper.cpp"),
    L("config/application.ini"),
    L("config/database.ini"),
    L("config/development.ini"),
    L("config/internet_media_types.ini"),
    L("config/logger.ini"),
    L("config/mongodb.ini"),
    L("config/redis.ini"),
    L("config/memcached.ini"),
    L("config/routes.cfg"),
    L("config/validation.ini"),
    L("config/cache.ini"),
    L("public/403.html"),
    L("public/404.html"),
    L("public/413.html"),
    L("public/500.html"),
    L("script/JSXTransformer.js"),
    L("script/react.js"),  // React
    L("script/react-with-addons.js"),  // React
    L("script/react-dom-server.js"),  // React
    // CMake
    L("CMakeLists.txt"),
    L("cmake/CacheClean.cmake"),
    L("cmake/TargetCmake.cmake"),
    L("controllers/CMakeLists.txt"),
    L("models/CMakeLists.txt"),
    L("views/CMakeLists.txt"),
    L("helpers/CMakeLists.txt"),
};

namespace {

const QString appIni = QLatin1String("config/application.ini");
const QString devIni = QLatin1String("config/development.ini");
QSettings appSettings(appIni, QSettings::IniFormat);
QSettings devSettings(devIni, QSettings::IniFormat);
TemplateSystem templateSystem = TemplateSystem::Invalid;


void usage()
{
    std::printf("usage: tspawn <subcommand> [args]\n\n"
                "Type 'tspawn --show-drivers' to show all the available database drivers for Qt.\n"
                "Type 'tspawn --show-driver-path' to show the path of database drivers for Qt.\n"
                "Type 'tspawn --show-tables' to show all tables to user in the setting of 'dev'.\n"
                "Type 'tspawn --show-collections' to show all collections in the MongoDB.\n\n"
                "Available subcommands:\n"
                "  new (n)         <application-name> [--template [erb | otama | vue | vite+vue]]\n"
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
                "  delete (d)      <table-name, helper-name or validator-name>\n"
    );
}


QStringList rmfiles(const QStringList &files, bool &allRemove, bool &quit, const QString &baseDir, const QString &proj = QString())
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


QStringList rmfiles(const QStringList &files, const QString &baseDir, const QString &proj)
{
    bool allRemove = false;
    bool quit = false;
    return rmfiles(files, allRemove, quit, baseDir, proj);
}


uint random(uint max)
{
    static std::random_device randev;
    static std::default_random_engine eng(randev());
    std::uniform_int_distribution<uint64_t> uniform(0, max);
    return uniform(eng);
}


QByteArray randomString(int length)
{
    constexpr auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray ret;
    int max = (int)strlen(ch) - 1;

    for (int i = 0; i < length; ++i) {
        ret += ch[random(max)];
    }
    return ret;
}


bool createNewApplication(const QString &name, const QByteArray &templateSystem)
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
    for (const QString &str : subDirs) {
        QString d = name + "/" + str;
        if (!mkpath(dir, d)) {
            return false;
        }
    }

    // Copies files
    copy(dataDirPath + "app.pro", name + "/" + name + ".pro");

    for (auto &path : filePaths) {
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

        // Replaces a string in development.ini file
        if (filename == "development.ini") {
            replaceString(dst, "$TemplateSystem$", templateSystem);
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


int deleteScaffold(const QString &name)
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
        if (templateSystem == TemplateSystem::Otama) {
            views << str + "/index.html"
                  << str + "/index.otm"
                  << str + "/show.html"
                  << str + "/show.otm"
                  << str + "/create.html"
                  << str + "/create.otm"
                  << str + "/save.html"
                  << str + "/save.otm";
        } else if (templateSystem == TemplateSystem::Erb || templateSystem == TemplateSystem::Vue
            || templateSystem == TemplateSystem::Vite_Vue) {
            views << str + "/index.erb"
                  << str + "/show.erb"
                  << str + "/create.erb"
                  << str + "/save.erb";
        } else {
            qCritical("Invalid template system specified");
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


bool checkIniFile()
{
    // Checking INI file
    if (!QFile::exists(appIni)) {
        qCritical("INI file not found, %s", qUtf8Printable(appIni));
        qCritical("Execute %s command in application root directory!", qUtf8Printable(QCoreApplication::arguments().value(0)));
        return false;
    }
    return true;
}


void printSuccessMessage(const QString &model)
{
    QString msg;

    if (!QFile("Makefile").exists() && !QFile(L("build/Makefile")).exists()) {
        msg = "qmake:\n Run `qmake -r%0 CONFIG+=debug` to generate Makefile for debug mode.\n Run `qmake -r%0 CONFIG+=release` to generate Makefile for release mode.\n";
#ifdef Q_OS_DARWIN
        msg = msg.arg(" -spec macx-clang");
#else
        msg = msg.arg("");
#endif
        msg += "\n or\n\ncmake:\n";
        msg += " Run `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` to generate Makefile for debug mode.\n";
        msg += " Run `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` to generate Makefile for release mode.\n";
        msg += " Run `cmake --build build --target cmake` to regenerate Makefile.";
    }

    putchar('\n');
    int port = appSettings.value("ListenPort").toInt();
    if (port > 0 && port <= USHRT_MAX)
        std::printf(" Index page URL:  http://localhost:%d/%s/index\n\n", port, qUtf8Printable(model));

    if (!msg.isEmpty()) {
        puts(qUtf8Printable(msg));
    }
}


std::unique_ptr<Generator> createServiceGenerator(TemplateSystem templateSystem, const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx)
{
    if (templateSystem == TemplateSystem::Erb) {
        return std::make_unique<ServiceGenerator>(service, fields, pkIdx, lockRevIdx);
    } else if (templateSystem == TemplateSystem::Vue) {
        return std::make_unique<VueServiceGenerator>(service, fields, pkIdx, lockRevIdx);
    } else if (templateSystem == TemplateSystem::Vite_Vue) {
        return std::make_unique<ViteVueServiceGenerator>(service, fields, pkIdx, lockRevIdx);
    } else if (templateSystem == TemplateSystem::Otama) {
        return std::make_unique<ServiceGenerator>(service, fields, pkIdx, lockRevIdx);
    } else {
        qCritical("Invalid template system specified");
        return nullptr;
    }
}


std::unique_ptr<Generator> createViewGenerator(TemplateSystem templateSystem, const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx)
{
    if (templateSystem == TemplateSystem::Erb) {
        return std::make_unique<ErbGenerator>(view, fields, pkIdx, autoValIdx);
    } else if (templateSystem == TemplateSystem::Vue) {
        return std::make_unique<VueErbGenerator>(view, fields, pkIdx, autoValIdx);
    } else if (templateSystem == TemplateSystem::Vite_Vue) {
        return std::make_unique<ViteVueGenerator>(view, fields, pkIdx, autoValIdx);
    } else if (templateSystem == TemplateSystem::Otama) {
        return std::make_unique<OtamaGenerator>(view, fields, pkIdx, autoValIdx);
    } else {
        qCritical("Invalid template system specified");
        return nullptr;
    }
}

}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = QCoreApplication::arguments();
    SubCommand subcmd = subCommands.value(args.value(1), SubCommand::Invalid);

    switch (subcmd) {
    case SubCommand::Invalid:
        qCritical("invalid argument");
        return 1;
        break;

    case SubCommand::Help:
        usage();
        break;

    case SubCommand::New: {
        // Creates new project
        QByteArray ts = "erb";
        if (args.count() > 4 && args.value(3) == "--template") {
            const auto name = args.value(4).toLower();
            if (templateSystemMap.contains(name)) {
                ts = name.toLatin1();
            }
        }

        if (!createNewApplication(args.value(2), ts)) {
            return 1;
        }
        break;
    }

    case SubCommand::ShowDrivers:
        std::printf("Available database drivers for Qt:\n");
        for (QStringListIterator i(TableSchema::databaseDrivers()); i.hasNext();) {
            std::printf("  %s\n", qUtf8Printable(i.next()));
        }
        break;

    case SubCommand::ShowDriverPath: {
        QString path = QLibraryInfo::path(QLibraryInfo::PluginsPath) + "/sqldrivers";
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isDir()) {
            qCritical("Error: database driver's directory not found");
            return 1;
        }
        std::printf("%s\n", qUtf8Printable(fi.canonicalFilePath()));
        break;
    }

    case SubCommand::ShowTables:
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

    case SubCommand::ShowCollections:
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

        // Template system
        QString ts = devSettings.value("TemplateSystem").toString().toLower();
        if (ts.isEmpty()) {
            ts = appSettings.value("TemplateSystem", "Erb").toString().toLower();
        }

        if (!templateSystemMap.contains(ts)) {
            qCritical("Invalid template system specified: %s", qUtf8Printable(ts));
            return 2;
        } else {
           templateSystem = templateSystemMap.value(ts);
        }

        switch (subcmd) {
        case SubCommand::Controller: {
            QString ctrl = args.value(2);
            ControllerGenerator crtlgen(ctrl, args.mid(3));
            crtlgen.generate(D_CTRLS);

            // Create view directory
            QDir dir(D_VIEWS + ((ctrl.contains('_')) ? ctrl.toLower() : fieldNameToVariableName(ctrl).toLower()));
            mkpath(dir, ".");
            copy(dataDirPath + ".trim_mode", dir);
            break;
        }

        case SubCommand::Model: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(3), args.value(2));
            modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            // Generates servie file of the specified template system
            std::unique_ptr<Generator> svrgen = createServiceGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            if (svrgen) {
                svrgen->generate(D_MODELS);
            }
            break;
        }

        case SubCommand::Helper: {
            HelperGenerator helpergen(args.value(2));
            helpergen.generate(D_HELPERS);
            break;
        }

        case SubCommand::UserModel: {
            ModelGenerator modelgen(ModelGenerator::Sql, args.value(5), args.value(2), args.mid(3, 2));
            modelgen.generate(D_MODELS, true);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            // Generates servie file of the specified template system
            std::unique_ptr<Generator> svrgen = createServiceGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            if (svrgen) {
                svrgen->generate(D_MODELS);
            }
            break;
        }

        case SubCommand::SqlObject: {
            SqlObjGenerator sqlgen(args.value(3), args.value(2));
            QString path = sqlgen.generate(D_MODELS);

            // Generates a project file
            ProjectFileGenerator progen(D_MODELS + "models.pro");
            progen.add(QStringList(path));
            break;
        }

        case SubCommand::MongoScaffold: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            bool success = modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            // Generates view files of the specified template system
            std::unique_ptr<Generator> svrgen = createServiceGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            if (svrgen) {
                svrgen->generate(D_MODELS);
            } else {
                qCritical("Invalid template system specified");
                return 2;
            }

            ControllerGenerator crtlgen(modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.lockRevisionIndex());
            success &= crtlgen.generate(D_CTRLS);

            // Generates view files of the specified template system
            std::unique_ptr<Generator> viewgen = createViewGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.autoValueIndex());
            if (viewgen) {
                viewgen->generate(D_VIEWS);
            } else {
                return 2;
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break;
        }

        case SubCommand::MongoModel: {
            ModelGenerator modelgen(ModelGenerator::Mongo, args.value(2));
            modelgen.generate(D_MODELS);

            int pkidx = modelgen.primaryKeyIndex();
            if (pkidx < 0) {
                qWarning("Primary key not found. [table name: %s]", qUtf8Printable(args.value(2)));
                return 2;
            }

            // Generates servie file of the specified template system
            std::unique_ptr<Generator> svrgen = createServiceGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            if (svrgen) {
                svrgen->generate(D_MODELS);
            }
            break;
        }

        case SubCommand::WebSocketEndpoint: {
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

        case SubCommand::Api: {
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

        case SubCommand::Validator: {
            ValidatorGenerator validgen(args.value(2));
            validgen.generate(D_HELPERS);
            break;
        }

        case SubCommand::Mailer: {
            MailerGenerator mailgen(args.value(2), args.mid(3));
            mailgen.generate(D_CTRLS);
            copy(dataDirPath + "mail.erb", D_VIEWS + "mailer/mail.erb");
            break;
        }

        case SubCommand::Scaffold: {
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

            // Generates service file of the specified template system
            std::unique_ptr<Generator> svrgen = createServiceGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), pkidx, modelgen.lockRevisionIndex());
            if (svrgen) {
                svrgen->generate(D_MODELS);
            } else {
                qCritical("Invalid template system specified");
                return 2;
            }

            // Generates view files of the specified template system
            std::unique_ptr<Generator> viewgen = createViewGenerator(templateSystem, modelgen.model(), modelgen.fieldList(), modelgen.primaryKeyIndex(), modelgen.autoValueIndex());
            if (viewgen) {
                viewgen->generate(D_VIEWS);
            } else {
                qCritical("Invalid template system specified");
                return 2;
            }

            if (success) {
                printSuccessMessage(modelgen.model());
            }
            break;
        }

        case SubCommand::Delete: {
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
