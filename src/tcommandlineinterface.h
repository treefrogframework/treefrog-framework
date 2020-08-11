#pragma once
#include <QtCore>
#include <TActionThread>
#include <TGlobal>
#include <TKvsDatabasePool>
#include <TSqlDatabasePool>
#include <TSystemGlobal>
#include <TWebApplication>


#define TF_CLI_MAIN(STATICFUNCTION)                                                                                 \
    int main(int argc, char *argv[])                                                                                \
    {                                                                                                               \
        class Thread : public TActionThread {                                                                       \
        public:                                                                                                     \
            Thread() : TActionThread(0), returnCode(0) { }                                                          \
            volatile int returnCode;                                                                                \
                                                                                                                    \
        protected:                                                                                                  \
            virtual void run()                                                                                      \
            {                                                                                                       \
                returnCode = STATICFUNCTION();                                                                      \
                commitTransactions();                                                                               \
                for (QMap<int, QSqlDatabase>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) { \
                    it.value().close(); /* close SQL database */                                                    \
                }                                                                                                   \
                for (QMap<int, TKvsDatabase>::iterator it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) { \
                    it.value().close(); /* close KVS database */                                                    \
                }                                                                                                   \
                QEventLoop eventLoop;                                                                               \
                while (eventLoop.processEvents()) { }                                                               \
            }                                                                                                       \
        };                                                                                                          \
        TWebApplication app(argc, argv);                                                                            \
        QByteArray codecName = app.appSettings().value("InternalEncoding", "UTF-8").toByteArray();                  \
        QTextCodec *codec = QTextCodec::codecForName(codecName);                                                    \
        QTextCodec::setCodecForLocale(codec);                                                                       \
        tSetupSystemLogger();                                                                                       \
        tSetupQueryLogger();                                                                                        \
        tSetupAppLoggers();                                                                                         \
        int idx = QCoreApplication::arguments().indexOf("-e");                                                      \
        QString env = (idx > 0) ? QCoreApplication::arguments().value(idx + 1) : QString("product");                \
        app.setDatabaseEnvironment(env);                                                                            \
        Thread thread;                                                                                              \
        QObject::connect(&thread, SIGNAL(finished()), &app, SLOT(quit()));                                          \
        thread.start();                                                                                             \
        app.exec();                                                                                                 \
        tReleaseAppLoggers();                                                                                       \
        tReleaseQueryLogger();                                                                                      \
        tReleaseSystemLogger();                                                                                     \
        return thread.returnCode;                                                                                   \
    }

