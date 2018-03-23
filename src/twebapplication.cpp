/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QTextCodec>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <TWebApplication>
#include <TSystemGlobal>
#include <TAppSettings>
#include <cstdlib>
#include <thread>  // for hardware_concurrency()
#include "tdatabasecontextmainthread.h"

#define DEFAULT_INTERNET_MEDIA_TYPE   "text/plain"
#define DEFAULT_DATABASE_ENVIRONMENT  "product"


static QTextCodec *searchCodec(const char *name)
{
    QTextCodec *c = QTextCodec::codecForName(name);
    return (c) ? c : QTextCodec::codecForLocale();
}


/*!
  \class TWebApplication
  \brief The TWebApplication class provides an event loop for
  TreeFrog applications.
*/

/*!
  Constructor.
*/
TWebApplication::TWebApplication(int &argc, char **argv)
#ifdef TF_USE_GUI_MODULE
    : QApplication(argc, argv),
#else
    : QCoreApplication(argc, argv),
#endif
      dbEnvironment(DEFAULT_DATABASE_ENVIRONMENT)
{
#if defined(Q_OS_WIN)
    installNativeEventFilter(new TNativeEventFilter);
#endif

    // parse command-line args
    webRootAbsolutePath = ".";
    QStringList args = arguments();
    args.removeFirst();
    for (QStringListIterator i(args); i.hasNext(); ) {
        const QString &arg = i.next();
        if (arg.startsWith('-')) {
            if (arg == "-e" && i.hasNext()) {
                dbEnvironment = i.next();
            }
            if (arg == "-i" && i.hasNext()) {
                appServerId = i.next().toInt();
            }
        } else {
            if (QDir(arg).exists()) {
                webRootAbsolutePath = arg;
                if (!webRootAbsolutePath.endsWith(QDir::separator()))
                    webRootAbsolutePath += QDir::separator();
            }
        }
    }

    QDir webRoot(webRootAbsolutePath);
    if (webRoot.exists()) {
        webRootAbsolutePath = webRoot.absolutePath() + QDir::separator();
    }

    // Sets application name
    QString appName = QDir(webRootAbsolutePath).dirName();
    if (!appName.isEmpty()) {
        setApplicationName(appName);
    }

    // Creates settings objects
    TAppSettings::instantiate(appSettingsFilePath());
    loggerSetting = new QSettings(configPath() + "logger.ini", QSettings::IniFormat, this);
    validationSetting = new QSettings(configPath() + "validation.ini", QSettings::IniFormat, this);
    // Internet media types
    if (QFileInfo(configPath() + "internet_media_types.ini").exists()) {
        mediaTypes = new QSettings(configPath() + "internet_media_types.ini", QSettings::IniFormat, this);
    } else {
        mediaTypes = new QSettings(configPath() + "initializers" + QDir::separator() + "internet_media_types.ini", QSettings::IniFormat, this);
    }
    // Gets codecs
    codecInternal = searchCodec(Tf::appSettings()->value(Tf::InternalEncoding).toByteArray().trimmed().data());
    codecHttp = searchCodec(Tf::appSettings()->value(Tf::HttpOutputEncoding).toByteArray().trimmed().data());

    // Sets codecs for INI files
    loggerSetting->setIniCodec(codecInternal);
    validationSetting->setIniCodec(codecInternal);
    mediaTypes->setIniCodec(codecInternal);

    // SQL DB settings
    QString dbsets = Tf::appSettings()->value(Tf::SqlDatabaseSettingsFiles).toString().trimmed();
    if (dbsets.isEmpty()) {
        dbsets = Tf::appSettings()->readValue("DatabaseSettingsFiles").toString().trimmed();
    }
    const QStringList files = dbsets.split(QLatin1Char(' '), QString::SkipEmptyParts);
    for (auto &f : files) {
        QSettings *set = new QSettings(configPath() + f, QSettings::IniFormat, this);
        set->setIniCodec(codecInternal);
        sqlSettings.append(set);
    }

    // MongoDB settings
    QString mongoini = Tf::appSettings()->value(Tf::MongoDbSettingsFile).toString().trimmed();
    if (!mongoini.isEmpty()) {
        QString mnginipath = configPath() + mongoini;
        if (QFile(mnginipath).exists())
            mongoSetting = new QSettings(mnginipath, QSettings::IniFormat, this);
    }

    // Redis settings
    QString redisini = Tf::appSettings()->value(Tf::RedisSettingsFile).toString().trimmed();
    if (!redisini.isEmpty()) {
        QString redisinipath = configPath() + redisini;
        if (QFile(redisinipath).exists())
            redisSetting = new QSettings(redisinipath, QSettings::IniFormat, this);
    }
}


TWebApplication::~TWebApplication()
{ }

/*!
  Enters the main event loop and waits until exit() is called. Returns the
  value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int TWebApplication::exec()
{
    resetSignalNumber();

#ifdef TF_USE_GUI_MODULE
    int ret = QApplication::exec();
#else
    int ret = QCoreApplication::exec();
#endif

    QEventLoop eventLoop;
    while (eventLoop.processEvents()) { }
    return ret;
}

/*!
  Returns true if the web root directory exists; otherwise returns false.
*/
bool TWebApplication::webRootExists() const
{
    return !webRootAbsolutePath.isEmpty() && QDir(webRootAbsolutePath).exists();
}

/*!
  Returns the absolute path of the public directory.
*/
QString TWebApplication::publicPath() const
{
    return webRootPath() + "public" + QDir::separator();
}

/*!
  Returns the absolute path of the config directory.
*/
QString TWebApplication::configPath() const
{
    return webRootPath() + "config" + QDir::separator();
}

/*!
  Returns the absolute path of the library directory.
*/
QString TWebApplication::libPath() const
{
    return webRootPath()+ "lib" + QDir::separator();
}

/*!
  Returns the absolute path of the log directory.
*/
QString TWebApplication::logPath() const
{
    return webRootPath() + "log" + QDir::separator();
}

/*!
  Returns the absolute path of the plugin directory.
*/
QString TWebApplication::pluginPath() const
{
    return webRootPath() + "plugin" + QDir::separator();
}

/*!
  Returns the absolute path of the tmp directory.
*/
QString TWebApplication::tmpPath() const
{
    return webRootPath() + "tmp" + QDir::separator();
}

/*!
  Returns true if the file of the application settings exists;
  otherwise returns false.
*/
bool TWebApplication::appSettingsFileExists() const
{
    return !Tf::appSettings()->appIniSettings->allKeys().isEmpty();
}

/*!
  Returns the absolute file path of the application settings.
*/
QString TWebApplication::appSettingsFilePath() const
{
    return configPath() + "application.ini";
}

/*!
  Returns a reference to the QSettings object for settings of the
  SQL database \a databaseId.
*/
QSettings &TWebApplication::sqlDatabaseSettings(int databaseId) const
{
    return *sqlSettings[databaseId];
}

/*!
  Returns the number of SQL database settings files set by the setting
  \a DatabaseSettingsFiles in the application.ini.
*/
int TWebApplication::sqlDatabaseSettingsCount() const
{
    return sqlSettings.count();
}

/*!
  Returns true if SQL database is available; otherwise returns false.
*/
bool TWebApplication::isSqlDatabaseAvailable() const
{
    return sqlSettings.count() > 0;
}

/*!
  Returns a reference to the QSettings object for settings of the
  MongoDB system.
*/
QSettings &TWebApplication::mongoDbSettings() const
{
    return *mongoSetting;
}

/*!
  Returns true if MongoDB settings is available; otherwise returns false.
*/
bool TWebApplication::isMongoDbAvailable() const
{
    return (bool)mongoSetting;
}

/*!
  Returns a reference to the QSettings object for settings of the
  Redis system.
*/
QSettings &TWebApplication::redisSettings() const
{
    return *redisSetting;
}

/*!
  Returns true if Redis settings is available; otherwise returns false.
*/
bool TWebApplication::isRedisAvailable() const
{
    return (bool)redisSetting;
}

/*!
  Returns the internet media type associated with the file extension
  \a ext.
*/
QByteArray TWebApplication::internetMediaType(const QString &ext, bool appendCharset)
{
    if (ext.isEmpty())
        return QByteArray();

    QString type = mediaTypes->value(ext, DEFAULT_INTERNET_MEDIA_TYPE).toString();
    if (appendCharset && type.startsWith("text", Qt::CaseInsensitive)) {
        type += "; charset=" + Tf::app()->codecForHttpOutput()->name();
    }
    return type.toLatin1();
}

/*!
  Returns the error message for validation of the given \a rule. These messages
  are defined in the validation.ini.
*/
QString TWebApplication::validationErrorMessage(int rule) const
{
    validationSetting->beginGroup("ErrorMessage");
    QString msg = validationSetting->value(QString::number(rule)).toString();
    validationSetting->endGroup();
    return msg;
}

/*!
  Returns the module name for multi-processing that is set by the setting
  \a MultiProcessingModule in the application.ini.
*/
TWebApplication::MultiProcessingModule TWebApplication::multiProcessingModule() const
{
    if (mpm == Invalid) {
        QString str = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
        if (str == "thread") {
            mpm = Thread;
        } else if (str == "hybrid") {
#ifdef Q_OS_LINUX
            mpm = Hybrid;
#else
            tSystemWarn("Unsupported MPM: hybrid  (Linux only)");
            tWarn("Unsupported MPM: hybrid  (Linux only)");
            mpm = Thread;
#endif
        } else {
            tSystemWarn("Unsupported MPM: %s", qPrintable(str));
            tWarn("Unsupported MPM: %s", qPrintable(str));
            mpm = Thread;
        }
    }
    return mpm;
}

/*!
  Returns the maximum number of application servers, which is set in the
  application.ini.
*/
int TWebApplication::maxNumberOfAppServers() const
{
    QString mpmstr = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
    int num = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpmstr + ".MaxAppServers").toInt();
    if (num <= 0) {
        num = qMax(std::thread::hardware_concurrency(), (uint)1);
        tSystemWarn("Sets max number of AP servers to %d", num);
    }
    return num;
}

/*!
  Maximum number of action threads allowed to start simultaneously
  per server process.
*/
int TWebApplication::maxNumberOfThreadsPerAppServer() const
{
    int maxNum = 0;
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread:
        maxNum = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
        if (maxNum <= 0) {
            maxNum = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxServers", "128").toInt();
        }
        break;

    case TWebApplication::Hybrid:
        maxNum = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerAppServer").toInt();
        if (maxNum <= 0) {
            maxNum = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerServer", "128").toInt();
        }
        break;

    default:
        break;
    }

    return maxNum;
}

/*!
  Returns the absolute file path of the routes config.
*/
QString TWebApplication::routesConfigFilePath() const
{
    return configPath() + "routes.cfg";
}

/*!
  Returns the absolute file path of the system log, which is set by the
  setting \a SystemLog.FilePath in the application.ini.
*/
QString TWebApplication::systemLogFilePath() const
{
    QFileInfo fi(Tf::appSettings()->value(Tf::SystemLogFilePath, "log/treefrog.log").toString());
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}

/*!
  Returns the absolute file path of the access log, which is set by the
  setting \a AccessLog.FilePath in the application.ini.
*/
QString TWebApplication::accessLogFilePath() const
{
    QString name = Tf::appSettings()->value(Tf::AccessLogFilePath).toString().trimmed();
    if (name.isEmpty())
        return name;

    QFileInfo fi(name);
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}

/*!
  Returns the absolute file path of the SQL query log, which is set by the
  setting \a SqlQueryLogFile in the application.ini.
*/
QString TWebApplication::sqlQueryLogFilePath() const
{
    QString path = Tf::appSettings()->value(Tf::SqlQueryLogFile).toString();
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        path = (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
    }
    return path;
}


void TWebApplication::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        if (signalNumber() >= 0) {
            tSystemDebug("TWebApplication trapped signal  number:%d", signalNumber());
            //timer.stop();   /* Don't stop this timer */
            exit(signalNumber());
        }
    } else {
#ifdef TF_USE_GUI_MODULE
        QApplication::timerEvent(event);
#else
        QCoreApplication::timerEvent(event);
#endif
    }
}


QThread *TWebApplication::databaseContextMainThread() const
{
    static TDatabaseContextMainThread databaseThread;
    databaseThread.start();
    return &databaseThread;
}


const QVariantMap &TWebApplication::getConfig(const QString &configName)
{
    auto cnf = configName.toLower();

    if (!configMap.contains(cnf)) {
        QDir dir(configPath());
        QStringList filters = { configName + ".*", configName };
        const auto filist = dir.entryInfoList(filters);

        if (filist.isEmpty()) {
            tSystemWarn("No such config, %s", qPrintable(configName));
        } else {
            for (auto &fi : filist) {
                auto suffix = fi.completeSuffix().toLower();
                if (suffix == "ini") {
                    // INI format
                    QVariantMap map;
                    QSettings settings(fi.absoluteFilePath(), QSettings::IniFormat);
                    for (auto &k : (const QStringList &)settings.allKeys()) {
                        map.insert(k, settings.value(k));
                    }
                    configMap.insert(cnf, map);
                    break;

                } else if (suffix == "json") {
                    // JSON format
                    QFile jsonFile(fi.absoluteFilePath());
                    if (jsonFile.open(QIODevice::ReadOnly)) {
                        auto json = QJsonDocument::fromJson(jsonFile.readAll()).object();
                        jsonFile.close();
                        configMap.insert(cnf, json.toVariantMap());
                        break;
                    }

                } else {
                    tSystemWarn("Invalid format config, %s", qPrintable(fi.fileName()));
                }
            }
        }
    }
    return configMap[cnf];
}


QVariant TWebApplication::getConfigValue(const QString &configName, const QString &key, const QVariant &defaultValue)
{
    return getConfig(configName).value(key, defaultValue);
}


/*!
  \fn QString TWebApplication::webRootPath() const
  Returns the absolute path of the web root directory.
*/

/*!
  \fn QSettings &TWebApplication::appSettings() const
  Returns a reference to the QSettings object for settings of the
  web application, which file is the application.ini.
*/

/*!
  \fn QSettings &TWebApplication::loggerSettings () const
  Returns a reference to the QSettings object for settings of the
  logger, which file is logger.ini.
*/

/*!
  \fn QSettings &TWebApplication::validationSettings () const
  Returns a reference to the QSettings object for settings of the
  validation, which file is validation.ini.
*/

/*!
  \fn QTextCodec *TWebApplication::codecForInternal() const
  Returns a pointer to the codec used internally, which is set by the
  setting \a InternalEncoding in the application.ini. This codec is used
  by QObject::tr() and toLocal8Bit() functions.
*/

/*!
  \fn QTextCodec *TWebApplication::codecForHttpOutput() const
  Returns a pointer to the codec of the HTTP output stream used by an
  action view, which is set by the setting \a HttpOutputEncoding in
  the application.ini.
*/

/*!
  \fn QString TWebApplication::databaseEnvironment() const
  Returns the database environment, which string is used to load
  the settings in database.ini.
  \sa setDatabaseEnvironment(const QString &environment)
*/

/*!
  \fn void TWebApplication::watchConsoleSignal();
  Starts watching console signals i.e.\ registers a routine to handle the
  console signals.
*/

/*!
  \fn void TWebApplication::ignoreConsoleSignal();
  Ignores console signals, i.e.\ delivery of console signals will have no effect
  on the process.
*/

/*!
  \fn void TWebApplication::watchUnixSignal(int sig, bool watch);
  Starts watching the UNIX signal, i.e.\ registers a routine to handle the
  signal \a sig.
  \sa ignoreUnixSignal()
*/

/*!
  \fn void TWebApplication::ignoreUnixSignal(int sig, bool ignore)
  Ignores UNIX signals, i.e.\ delivery of the signal will have no effect on
  the process.
  \sa watchUnixSignal()
*/

/*!
  \fn void TWebApplication::timerEvent(QTimerEvent *)
  Reimplemented from QObject::timerEvent().
*/

/*!
  \fn int TWebApplication::signalNumber()
  Returns the integral number of the received signal.
*/
