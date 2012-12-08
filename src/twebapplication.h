#ifndef TWEBAPPLICATION_H
#define TWEBAPPLICATION_H

#ifdef TF_USE_GUI_MODULE
# include <QApplication>
#else
# include <QCoreApplication>
#endif

#include <QVector>
#include <QSettings>
#include <QBasicTimer>
#include <TGlobal>
#include "qplatformdefs.h"

class QTextCodec;


class T_CORE_EXPORT TWebApplication
#ifdef TF_USE_GUI_MODULE
    : public QApplication
#else
    : public QCoreApplication
#endif
{
public:
    enum MultiProcessingModule {
        Invalid = 0,
        Thread,
        Prefork,
    };
    
    TWebApplication(int &argc, char **argv);
    ~TWebApplication();

    int exec();
    QString webRootPath() const { return webRootAbsolutePath; }
    bool webRootExists() const;
    QString publicPath() const;
    QString configPath() const;
    QString libPath() const;
    QString logPath() const;
    QString pluginPath() const;
    QString tmpPath() const;
    QString databaseEnvironment() const { return dbEnvironment; }
    void setDatabaseEnvironment(const QString &environment);
    
    bool appSettingsFileExists() const;
    QString appSettingsFilePath() const;
    QSettings &appSettings() const { return *appSetting; }
    QSettings &databaseSettings(int databaseId) const;
    int databaseSettingsCount() const;
    bool isValidDatabaseSettings() const;
    QSettings &loggerSettings() const { return *loggerSetting; }
    QSettings &validationSettings() const { return *validationSetting; }
    QString validationErrorMessage(int rule) const;
    QByteArray internetMediaType(const QString &ext, bool appendCharset = false);
    MultiProcessingModule multiProcessingModule() const;
    int maxNumberOfServers() const;
    QString routesConfigFilePath() const;
    QString systemLogFilePath() const;
    QString accessLogFilePath() const;
    QString sqlQueryLogFilePath() const;
    QTextCodec *codecForInternal() const { return codecInternal; }
    QTextCodec *codecForHttpOutput() const { return codecHttp; }

#if defined(Q_OS_WIN)
    virtual bool winEventFilter(MSG *msg, long *result);
    void watchConsoleSignal();
    void ignoreConsoleSignal();
#else
    void watchUnixSignal(int sig, bool watch = true);
    void ignoreUnixSignal(int sig, bool ignore = true);
#endif

protected:
    void timerEvent(QTimerEvent *event);
    static int signalNumber();

private:
    Q_DISABLE_COPY(TWebApplication)

    QString webRootAbsolutePath;
    QString dbEnvironment;
    QSettings *appSetting;
    QVector<QSettings *> dbSettings;
    QSettings *loggerSetting;
    QSettings *validationSetting;
    QSettings *mediaTypes;
    QTextCodec *codecInternal;
    QTextCodec *codecHttp;
    QBasicTimer timer;
    mutable MultiProcessingModule mpm;

    static void resetSignalNumber();
};


inline void TWebApplication::setDatabaseEnvironment(const QString &environment)
{
    dbEnvironment = environment;
}

#endif // TWEBAPPLICATION_H
