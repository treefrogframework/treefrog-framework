/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachefactory.h"
#include "tdatabasecontextmainthread.h"
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextCodec>
#include <TAppSettings>
#include <TSystemGlobal>
#include <TWebApplication>
#include <cstdlib>
#include <thread>  // for hardware_concurrency()

constexpr auto DEFAULT_INTERNET_MEDIA_TYPE = "text/plain";
constexpr auto DEFAULT_DATABASE_ENVIRONMENT = "product";

namespace {
QTextCodec *searchCodec(const char *name)
{
    QTextCodec *c = QTextCodec::codecForName(name);
    return (c) ? c : QTextCodec::codecForLocale();
}
}

/*!
  \class TWebApplication
  \brief The TWebApplication class provides an event loop for
  TreeFrog applications.
*/

/*!
  Constructor.
*/
TWebApplication::TWebApplication(int &argc, char **argv) :
#ifdef TF_USE_GUI_MODULE
    QApplication(argc, argv),
#else
    QCoreApplication(argc, argv),
#endif
    _dbEnvironment(DEFAULT_DATABASE_ENVIRONMENT)
{
#if defined(Q_OS_WIN)
    installNativeEventFilter(new TNativeEventFilter);
#endif

    // parse command-line args
    _webRootAbsolutePath = ".";
    QStringList args = arguments();
    args.removeFirst();
    for (QStringListIterator i(args); i.hasNext();) {
        const QString &arg = i.next();
        if (arg.startsWith('-')) {
            if (arg == "-e" && i.hasNext()) {
                _dbEnvironment = i.next();
            }
            if (arg == "-i" && i.hasNext()) {
                _appServerId = i.next().toInt();
            }
        } else {
            if (QDir(arg).exists()) {
                _webRootAbsolutePath = arg;
                if (!_webRootAbsolutePath.endsWith("/")) {
                    _webRootAbsolutePath += "/";
                }
            }
        }
    }

    QDir webRoot(_webRootAbsolutePath);
    if (webRoot.exists()) {
        _webRootAbsolutePath = webRoot.absolutePath() + "/";
    }

    // Sets application name
    QString appName = QDir(_webRootAbsolutePath).dirName();
    if (!appName.isEmpty()) {
        setApplicationName(appName);
    }

    // Creates settings objects
    TAppSettings::instantiate(appSettingsFilePath());
    QSettings loggerSetting(configPath() + "logger.ini", QSettings::IniFormat, this);
    QSettings validationSetting(configPath() + "validation.ini", QSettings::IniFormat, this);
    // Internet media types
    QSettings *mediaTypes;
    if (QFileInfo(configPath() + "internet_media_types.ini").exists()) {
        mediaTypes = new QSettings(configPath() + "internet_media_types.ini", QSettings::IniFormat, this);
    } else {
        mediaTypes = new QSettings(configPath() + "initializers/internet_media_types.ini", QSettings::IniFormat, this);
    }
    // Gets codecs
    _codecInternal = searchCodec(Tf::appSettings()->value(Tf::InternalEncoding).toByteArray().trimmed().data());
    _codecHttp = searchCodec(Tf::appSettings()->value(Tf::HttpOutputEncoding).toByteArray().trimmed().data());

    // Sets codecs for INI files
#if QT_VERSION < 0x060000
    loggerSetting.setIniCodec(_codecInternal);
    validationSetting.setIniCodec(_codecInternal);
    mediaTypes->setIniCodec(_codecInternal);
#endif
    _loggerSetting = Tf::settingsToMap(loggerSetting);
    _validationSetting = Tf::settingsToMap(validationSetting);
    _mediaTypes = Tf::settingsToMap(*mediaTypes);
    delete mediaTypes;

    // SQL DB settings
    const QStringList files = []() {
        // delimiter: comma or space
        QStringList files;
        QStringList dbsets = Tf::appSettings()->value(Tf::SqlDatabaseSettingsFiles).toStringList();
        if (dbsets.isEmpty()) {
            dbsets = Tf::appSettings()->readValue("DatabaseSettingsFiles").toStringList();
        }
        for (auto &s : dbsets) {
            files << s.simplified().split(QLatin1Char(' '));
        }
        return files;
    }();

    for (auto &f : files) {
        QSettings settings(configPath() + f, QSettings::IniFormat);
#if QT_VERSION < 0x060000
        settings.setIniCodec(_codecInternal);
#endif
        _sqlSettings.append(Tf::settingsToMap(settings, _dbEnvironment));
    }

    // MongoDB settings
    QString mongoini = Tf::appSettings()->value(Tf::MongoDbSettingsFile).toString().trimmed();
    if (!mongoini.isEmpty()) {
        QString mnginipath = configPath() + mongoini;
        if (QFile(mnginipath).exists()) {
            QSettings settings(mnginipath, QSettings::IniFormat);
#if QT_VERSION < 0x060000
            settings.setIniCodec(_codecInternal);
#endif
            _kvsSettings[(int)Tf::KvsEngine::MongoDB] = Tf::settingsToMap(settings, _dbEnvironment);
        }
    }

    // Redis settings
    QString redisini = Tf::appSettings()->value(Tf::RedisSettingsFile).toString().trimmed();
    if (!redisini.isEmpty()) {
        QString redisinipath = configPath() + redisini;
        if (QFile(redisinipath).exists()) {
            QSettings settings(redisinipath, QSettings::IniFormat);
#if QT_VERSION < 0x060000
            settings.setIniCodec(_codecInternal);
#endif
            _kvsSettings[(int)Tf::KvsEngine::Redis] = Tf::settingsToMap(settings, _dbEnvironment);
        }
    }

    // Cache settings
    if (cacheEnabled()) {
        auto backend = cacheBackend();
        QString path = Tf::appSettings()->value(Tf::CacheSettingsFile).toString().trimmed();
        if (!path.isEmpty()) {
            QVariantMap settings = TCacheFactory::defaultSettings(backend);
            // Copy settings
            QSettings iniset(configPath() + path, QSettings::IniFormat);
            iniset.beginGroup(backend);
            for (auto &k : iniset.allKeys()) {
                auto val = iniset.value(k).toString().trimmed();
                if (!val.isEmpty()) {
                    settings.insert(k, iniset.value(k));
                }
            }

            if (TCacheFactory::dbType(backend) == TCacheStore::SQL) {
                _sqlSettings.append(settings);
                _cacheSqlDbIndex = _sqlSettings.count() - 1;
            } else if (TCacheFactory::dbType(backend) == TCacheStore::KVS) {
                _kvsSettings[(int)Tf::KvsEngine::CacheKvs] = settings;
            }
        }
    }
}


TWebApplication::~TWebApplication()
{
}

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
    while (eventLoop.processEvents()) {
    }
    return ret;
}

/*!
  Returns true if the web root directory exists; otherwise returns false.
*/
bool TWebApplication::webRootExists() const
{
    return !_webRootAbsolutePath.isEmpty() && QDir(_webRootAbsolutePath).exists();
}

/*!
  Returns the absolute path of the public directory.
*/
QString TWebApplication::publicPath() const
{
    return webRootPath() + "public/";
}

/*!
  Returns the absolute path of the config directory.
*/
QString TWebApplication::configPath() const
{
    return webRootPath() + "config/";
}

/*!
  Returns the absolute path of the library directory.
*/
QString TWebApplication::libPath() const
{
    return webRootPath() + "lib/";
}

/*!
  Returns the absolute path of the log directory.
*/
QString TWebApplication::logPath() const
{
    return webRootPath() + "log/";
}

/*!
  Returns the absolute path of the plugin directory.
*/
QString TWebApplication::pluginPath() const
{
    return webRootPath() + "plugin/";
}

/*!
  Returns the absolute path of the tmp directory.
*/
QString TWebApplication::tmpPath() const
{
    return webRootPath() + "tmp/";
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
const QVariantMap &TWebApplication::sqlDatabaseSettings(int databaseId) const
{
    static QVariantMap invalidSettings;
    return (databaseId >= 0 && databaseId < _sqlSettings.count()) ? _sqlSettings[databaseId] : invalidSettings;
}

/*!
  Returns the number of SQL database settings files set by the setting
  \a DatabaseSettingsFiles in the application.ini.
*/
int TWebApplication::sqlDatabaseSettingsCount() const
{
    return _sqlSettings.count();
}

/*!
 */
int TWebApplication::databaseIdForCache() const
{
    return _cacheSqlDbIndex;
}

/*!
  Returns a reference to the settings object for the specified engine.
*/
const QVariantMap &TWebApplication::kvsSettings(Tf::KvsEngine engine) const
{
    return _kvsSettings[(int)engine];
}

/*!
  Returns true if the settings of the specified engine is available;
  otherwise returns false.
*/
bool TWebApplication::isKvsAvailable(Tf::KvsEngine engine) const
{
    return !_kvsSettings[(int)engine].isEmpty();
}


/*!
  Returns the internet media type associated with the file extension
  \a ext.
*/
QByteArray TWebApplication::internetMediaType(const QString &ext, bool appendCharset)
{
    if (ext.isEmpty()) {
        return QByteArray();
    }

    QString type = _mediaTypes.value(ext.toLower(), DEFAULT_INTERNET_MEDIA_TYPE).toString();
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
    return _validationSetting.value("ErrorMessage/" + QString::number(rule)).toString();
}

/*!
  Returns the module name for multi-processing that is set by the setting
  \a MultiProcessingModule in the application.ini.
*/
TWebApplication::MultiProcessingModule TWebApplication::multiProcessingModule() const
{
    if (_mpm == Invalid) {
        QString str = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
        if (str == "thread") {
            _mpm = Thread;
        } else if (str == "epoll") {
#ifdef Q_OS_LINUX
            _mpm = Epoll;
#else
            tSystemWarn("Unsupported MPM: epoll  (Linux only)");
            tWarn("Unsupported MPM: epoll  (Linux only)");
            _mpm = Thread;
#endif
        } else {
            tSystemWarn("Unsupported MPM: %s", qUtf8Printable(str));
            tWarn("Unsupported MPM: %s", qUtf8Printable(str));
            _mpm = Thread;
        }
    }
    return _mpm;
}

/*!
  Returns the maximum number of application servers, which is set in the
  application.ini.
*/
int TWebApplication::maxNumberOfAppServers() const
{
    static const int maxServers = ([]() -> int {
        QString mpmstr = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
        int num = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpmstr + ".MaxAppServers").toInt();

        if (num <= 0) {
            num = qMax(std::thread::hardware_concurrency(), (uint)1);
            tSystemWarn("Sets max number of AP servers to %d", num);
        }
        return num;
    }());
    return maxServers;
}

/*!
  Maximum number of action threads allowed to start simultaneously
  per server process.
*/
int TWebApplication::maxNumberOfThreadsPerAppServer() const
{
    static int maxNumber = []() {
        int maxNum = 0;
        QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();

        switch (Tf::app()->multiProcessingModule()) {
        case TWebApplication::Thread:
            maxNum = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
            maxNum = (maxNum > 0) ? maxNum : 128;
            break;

        case TWebApplication::Epoll:
            maxNum = 128;
            break;

        default:
            break;
        }
        return maxNum;
    }();

    return maxNumber;
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
    if (event->timerId() == _timer.timerId()) {
        if (signalNumber() >= 0) {
            tSystemDebug("TWebApplication trapped signal  number:%d", signalNumber());
            //timer.stop();   /* Don't stop this timer */
            QCoreApplication::exit(signalNumber());
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
    static TDatabaseContextMainThread *databaseThread = []() {
        auto *thread = new TDatabaseContextMainThread;
        thread->start();
        return thread;
    }();
    return databaseThread;
}


const QVariantMap &TWebApplication::getConfig(const QString &configName)
{
    auto cnf = configName.toLower();

    if (!_configMap.contains(cnf)) {
        QDir dir(configPath());
        QStringList filters = {configName + ".*", configName};
        const auto filist = dir.entryInfoList(filters);

        if (filist.isEmpty()) {
            tSystemWarn("No such config, %s", qUtf8Printable(configName));
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
                    _configMap.insert(cnf, map);
                    break;

                } else if (suffix == "json") {
                    // JSON format
                    QFile jsonFile(fi.absoluteFilePath());
                    if (jsonFile.open(QIODevice::ReadOnly)) {
                        auto json = QJsonDocument::fromJson(jsonFile.readAll()).object();
                        jsonFile.close();
                        _configMap.insert(cnf, json.toVariantMap());
                        break;
                    }

                } else {
                    tSystemWarn("Invalid format config, %s", qUtf8Printable(fi.fileName()));
                }
            }
        }
    }
    return _configMap[cnf];
}


QVariant TWebApplication::getConfigValue(const QString &configName, const QString &key, const QVariant &defaultValue)
{
    return getConfig(configName).value(key, defaultValue);
}


bool TWebApplication::cacheEnabled() const
{
    static bool enable = !Tf::appSettings()->value(Tf::CacheSettingsFile).toString().trimmed().isEmpty();
    return enable;
}


QString TWebApplication::cacheBackend() const
{
    static QString backend = Tf::appSettings()->value(Tf::CacheBackend).toString().toLower();
    return backend;
}


/*!
  \fn QString TWebApplication::webRootPath() const
  Returns the absolute path of the web root directory.
*/

/*!
  \fn const QVariantMap &TWebApplication::appSettings() const
  Returns a reference to the QSettings object for settings of the
  web application, which file is the application.ini.
*/

/*!
  \fn const QVariantMap &TWebApplication::loggerSettings () const
  Returns a reference to the QSettings object for settings of the
  logger, which file is logger.ini.
*/

/*!
  \fn const QVariantMap &TWebApplication::validationSettings () const
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
