/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QTextCodec>
#include <TWebApplication>
#include <TSystemGlobal>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

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

TWebApplication::TWebApplication(int &argc, char **argv)
#ifdef TF_USE_GUI_MODULE
    : QApplication(argc, argv),
#else
    : QCoreApplication(argc, argv),
#endif
      dbEnvironment(DEFAULT_DATABASE_ENVIRONMENT),
      appSetting(0),
      dbSettings(0),
      loggerSetting(0),
      validationSetting(0),
      mediaTypes(0),
      codecInternal(0),
      codecHttp(0),
      mpm(Invalid)
{
#if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
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
    appSetting = new QSettings(appSettingsFilePath(), QSettings::IniFormat, this);
    loggerSetting = new QSettings(configPath() + "logger.ini", QSettings::IniFormat, this);
    validationSetting = new QSettings(configPath() + "validation.ini", QSettings::IniFormat, this);
    mediaTypes = new QSettings(configPath() + "initializers" + QDir::separator() + "internet_media_types.ini", QSettings::IniFormat, this);

    // Gets codecs
    codecInternal = searchCodec(appSetting->value("InternalEncoding").toByteArray().trimmed().data());
    codecHttp = searchCodec(appSetting->value("HttpOutputEncoding").toByteArray().trimmed().data());

    // Sets codecs for INI files
    loggerSetting->setIniCodec(codecInternal);
    validationSetting->setIniCodec(codecInternal);
    mediaTypes->setIniCodec(codecInternal);

    // DB settings
    QStringList files = appSetting->value("DatabaseSettingsFiles", "database.ini").toString().trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
    for (QListIterator<QString> it(files); it.hasNext(); ) {
        const QString &f = it.next();
        QSettings *set = new QSettings(configPath() + f, QSettings::IniFormat, this);
        set->setIniCodec(codecInternal);
        dbSettings.append(set);
    }
    
    // sets a seed for random numbers
    struct timeval tv;
    gettimeofday(&tv, NULL);
    Tf::srandXor128(((uint)tv.tv_sec << 14) | (getpid() & 0x3fff));
}


TWebApplication::~TWebApplication()
{ }


int TWebApplication::exec()
{
    resetSignalNumber();
#ifdef TF_USE_GUI_MODULE
    return QApplication::exec();
#else
    return QCoreApplication::exec();
#endif
}


bool TWebApplication::webRootExists() const
{
    return !webRootAbsolutePath.isEmpty() && QDir(webRootAbsolutePath).exists();
}


QString TWebApplication::publicPath() const
{
    return webRootPath() + "public" + QDir::separator();
}


QString TWebApplication::configPath() const
{
    return webRootPath() + "config" + QDir::separator();
}


QString TWebApplication::libPath() const
{
    return webRootPath()+ "lib" + QDir::separator();
}


QString TWebApplication::logPath() const
{
    return webRootPath() + "log" + QDir::separator();
}


QString TWebApplication::pluginPath() const
{
    return webRootPath() + "plugin" + QDir::separator();
}


QString TWebApplication::tmpPath() const
{
    return webRootPath() + "tmp" + QDir::separator();
}


bool TWebApplication::appSettingsFileExists() const
{
    return !appSetting->allKeys().isEmpty();
}


QString TWebApplication::appSettingsFilePath() const
{
    return configPath() + "application.ini";
}


QSettings &TWebApplication::databaseSettings(int databaseId) const
{
    return *dbSettings[databaseId];
}


int TWebApplication::databaseSettingsCount() const
{
    return dbSettings.count();
}


bool TWebApplication::isValidDatabaseSettings() const
{
    bool valid = false;
    for (int i = 0; i < dbSettings.count(); ++i) {
        QSettings *settings = dbSettings[i];
        settings->beginGroup(dbEnvironment);
        valid = !settings->childKeys().isEmpty();
        settings->endGroup();

        if (!valid)
            break;
    }
    return valid;
}


QByteArray TWebApplication::internetMediaType(const QString &ext, bool appendCharset)
{
    if (ext.isEmpty())
        return QByteArray();

    QString type = mediaTypes->value(ext, DEFAULT_INTERNET_MEDIA_TYPE).toString();
    if (appendCharset && type.startsWith("text", Qt::CaseInsensitive)) {
        type += "; charset=" + appSetting->value("HtmlContentCharset").toString();
    }
    return type.toLatin1();
}


QString TWebApplication::validationErrorMessage(int rule) const
{
    validationSetting->beginGroup("ErrorMessage");
    QString msg = validationSetting->value(QString::number(rule)).toString();
    validationSetting->endGroup();
    return msg;
}


TWebApplication::MultiProcessingModule TWebApplication::multiProcessingModule() const
{
    if (mpm == Invalid) {
        QString str = appSettings().value("MultiProcessingModule").toString().toLower();
        if (str == "thread") {
            mpm = Thread;
        } else if (str == "prefork") {
            mpm = Prefork;
        }
    }
    return mpm;
}


int TWebApplication::maxNumberOfServers() const
{
    QString mpm = appSettings().value("MultiProcessingModule").toString().toLower();
    int num = appSettings().value(QLatin1String("MPM.") + mpm + ".MaxServers").toInt();
    Q_ASSERT(num > 0);
    return num;
}


QString TWebApplication::routesConfigFilePath() const
{
    return configPath() + "routes.cfg";
}


QString TWebApplication::systemLogFilePath() const
{
    QFileInfo fi(appSettings().value("SystemLog.FilePath", "log/treefrog.log").toString());
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}


QString TWebApplication::accessLogFilePath() const
{
    QFileInfo fi(appSettings().value("AccessLog.FilePath", "log/access.log").toString());
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}


QString TWebApplication::sqlQueryLogFilePath() const
{
    QString path = appSettings().value("SqlQueryLogFile").toString();
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
