#ifndef TFTEST_H
#define TFTEST_H

#include <QtTest/QtTest>
#include <QObject>
#include <QTextCodec>
#include <QByteArray>
#include <QEventLoop>
#include <TWebApplication>
#include <TAppSettings>
#include "tkvsdatabasepool.h"
#ifdef QT_SQL_LIB
# include <TActionThread>
# include "tsqldatabasepool.h"
#endif


#define TF_TEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    class Thread : public TActionThread { \
    public: \
        Thread() : TActionThread(0), returnCode(0) { } \
        volatile int returnCode; \
    protected: \
        virtual void run() \
        { \
            TestObject obj; \
            returnCode = QTest::qExec(&obj, QCoreApplication::arguments()); \
            commitTransactions(); \
            for (QMap<int, QSqlDatabase>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) { \
                it.value().close(); /* close SQL database */ \
            } \
            for (QMap<int, TKvsDatabase>::iterator it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) { \
                it.value().close(); /* close KVS database */ \
            } \
            QEventLoop eventLoop; \
            while (eventLoop.processEvents()) {} \
        } \
    }; \
    TWebApplication app(argc, argv); \
    QByteArray codecName = Tf::appSettings()->value(Tf::InternalEncoding, "UTF-8").toByteArray(); \
    QTextCodec *codec = QTextCodec::codecForName(codecName); \
    QTextCodec::setCodecForLocale(codec); \
    app.setDatabaseEnvironment("test"); \
    TSqlDatabasePool::instantiate(); \
    TKvsDatabasePool::instantiate(); \
    Thread thread; \
    thread.start(); \
    thread.wait(); \
    _exit(thread.returnCode); \
    return thread.returnCode; \
}


#define TF_TEST_SQLLESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    TWebApplication app(argc, argv); \
    QByteArray codecName = Tf::appSettings()->value(Tf::InternalEncoding, "UTF-8").toByteArray(); \
    QTextCodec *codec = QTextCodec::codecForName(codecName); \
    QTextCodec::setCodecForLocale(codec); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // TFTEST_H
