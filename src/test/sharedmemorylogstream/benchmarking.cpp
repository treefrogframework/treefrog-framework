#include <TfTest/TfTest>
#include <TSystemGlobal>
#include "tsharedmemorylogstream.h"
#include "tbasiclogstream.h"
#include "tfilelogger.h"


class BenchMark : public QObject
{
    Q_OBJECT
private slots:
    void systemDebug();
    void smemNonBufferingWriteLog();
    void smemWriteLog();
    void basicNonBufferingWriteLog();
    void basicWriteLog();
    void rawWriteLog();
};


void BenchMark::systemDebug()
{
    tSetupSystemLoggers();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            tSystemInfo("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
        }
    }
}


void BenchMark::smemNonBufferingWriteLog()
{
    TFileLogger logger;
    QList<TLogger *> list;
    list << &logger;
    TSharedMemoryLogStream stream(list);
    stream.setNonBufferingMode();

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
    TLog log(1, ba);
    stream.writeLog(log);
    stream.flush();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);
            
            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
}


void BenchMark::smemWriteLog()
{
    TFileLogger logger;
    QList<TLogger *> list;
    list << &logger;
    TSharedMemoryLogStream stream(list);

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
    TLog log(1, ba);
    stream.writeLog(log);
    stream.flush();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);
            
            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
    stream.flush();
}


void BenchMark::basicNonBufferingWriteLog()
{
    TFileLogger logger;
    QList<TLogger *> list;
    list << &logger;
    TBasicLogStream stream(list);
    stream.setNonBufferingMode();

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
    TLog log(1, ba);
    stream.writeLog(log);
    stream.flush();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);
            
            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
    stream.flush();
}


void BenchMark::basicWriteLog()
{
    TFileLogger logger;
    QList<TLogger *> list;
    list << &logger;
    TBasicLogStream stream(list);

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
    TLog log(1, ba);
    stream.writeLog(log);
    stream.flush();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);
            
            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
    stream.flush();
}


void BenchMark::rawWriteLog()
{
    TFileLogger logger;
    logger.open();

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");
    TLog log(1, ba);

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            logger.log(log);
            
            if ((i + 1) % 5 == 0)
                logger.flush();
        }
    }
}


TF_TEST_MAIN(BenchMark)
#include "benchmarking.moc"
