#pragma once
#include "tkvsdatabasepool.h"
#include "turlroute.h"
#include <QByteArray>
#include <QEventLoop>
#include <QObject>
#include <QtTest/QtTest>
#include <TAppSettings>
#include <TSystemGlobal>
#include <TStdErrSystemLogger>
#include <TWebApplication>
#ifdef QT_SQL_LIB
#include "tsqldatabasepool.h"
#include <TActionThread>
#endif
#ifdef Q_OS_LINUX
#include <TMultiplexingServer>
#endif


#define TF_TEST_MAIN(TestObject) TF_TEST_SQL_MAIN(TestObject, true);

#define TF_TEST_SQL_MAIN(TestObject, EnableTransactions)                                                               \
    int main(int argc, char *argv[])                                                                                   \
    {                                                                                                                  \
        class Thread : public TActionThread {                                                                          \
        public:                                                                                                        \
            Thread() : TActionThread(0), returnCode(0) { }                                                             \
            volatile int returnCode;                                                                                   \
        protected:                                                                                                     \
            virtual void run()                                                                                         \
            {                                                                                                          \
                setTransactionEnabled(EnableTransactions);                                                             \
                TestObject obj;                                                                                        \
                returnCode = QTest::qExec(&obj, QCoreApplication::arguments());                                        \
                commitTransactions();                                                                                  \
                QEventLoop eventLoop;                                                                                  \
                while (eventLoop.processEvents()) {                                                                    \
                }                                                                                                      \
            }                                                                                                          \
        };                                                                                                             \
        TWebApplication app(argc, argv);                                                                               \
        Tf::setupSystemLogger(new TStdErrSystemLogger);                                                                \
        app.setDatabaseEnvironment("test");                                                                            \
        TUrlRoute::instance();                                                                                         \
        TSqlDatabasePool::instance();                                                                                  \
        TKvsDatabasePool::instance();                                                                                  \
        INIT_MULTIPLEXING_SERVER();                                                                                    \
        Thread thread;                                                                                                 \
        thread.start();                                                                                                \
        thread.wait();                                                                                                 \
        return thread.returnCode;                                                                                      \
    }


#define TF_TEST_SQLLESS_MAIN(TestObject)                                                              \
    int main(int argc, char *argv[])                                                                  \
    {                                                                                                 \
        TWebApplication app(argc, argv);                                                              \
        Tf::setupSystemLogger(new TStdErrSystemLogger);                                               \
        TestObject tc;                                                                                \
        return QTest::qExec(&tc, argc, argv);                                                         \
    }


#ifdef Q_OS_LINUX
#define INIT_MULTIPLEXING_SERVER() TMultiplexingServer::instantiate(0)
#else
#define INIT_MULTIPLEXING_SERVER()
#endif
