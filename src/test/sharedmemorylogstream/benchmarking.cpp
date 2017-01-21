#include <TfTest/TfTest>
#include <TSystemGlobal>
#include "tsharedmemorylogstream.h"
#include "tbasiclogstream.h"
#include "tfilelogger.h"
#include "tfileaiologger.h"
#include "tfileaiowriter.h"


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
    void aioWriteLog();
    void aioWriter();
    void cleanupTestCase();
};


void BenchMark::systemDebug()
{
    Tf::setupSystemLogger();

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


void BenchMark::aioWriteLog()
{
    TFileAioLogger logger;
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

void BenchMark::aioWriter()
{
    TFileAioWriter writer;
    writer.setFileName("log/app.log");
    writer.open();

    QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            writer.write(ba.data(), ba.length());

            if ((i + 1) % 5 == 0)
                writer.flush();
        }
    }
}


void BenchMark::cleanupTestCase()
{
    QDir logDir("log");
    QStringList flist = logDir.entryList(QDir::Files);
    for (QListIterator<QString> it(flist); it.hasNext(); ) {
        logDir.remove(it.next());
    }
    logDir.rmpath(".");
}



TF_TEST_MAIN(BenchMark)
#include "benchmarking.moc"
