#include <TfTest/TfTest>
#include <TSystemGlobal>
//#include "tsharedmemorylogstream.h"
#include "tbasiclogstream.h"
#include "tfilelogger.h"
#include "trash/tfileaiologger.h"
#include "trash/tfileaiowriter.h"


class BenchMark : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    //void systemDebug();
    //void sharedMemoryLogStream_nonBuffer();
    //void sharedMemoryLogStream();
    //void basicLogStream_nonBuffer();
    void basicLogStream();
    void fileLogger();
    void fileAioLogger();
    void aioWriter();
};


const QByteArray ba("aildjfliasjdl;fijaswelirjas;l;liajds;flkjuuuuuhhujijiji");

// void BenchMark::systemDebug()
// {
//      QBENCHMARK {
//         for (int i = 0; i < 10; ++i) {
//             tSystemInfo("{}", ba.data());
//         }
//     }
// }

/*
void BenchMark::sharedMemoryLogStream_nonBuffer()
{
    TFileLogger logger;
    logger.setFileName("log/shm-nonbuffer-logstream.log");
    QList<TLogger *> list;
    list << &logger;
    TSharedMemoryLogStream stream(list);
    stream.setNonBufferingMode();

    TLog log(1, ba);

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);

            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
}


void BenchMark::sharedMemoryLogStream()
{
    TFileLogger logger;
    logger.setFileName("log/shm-logstream.log");
    QList<TLogger *> list;
    list << &logger;
    TSharedMemoryLogStream stream(list);

    TLog log(1, ba);

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);

            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
    stream.flush();
}
*/

// void BenchMark::basicLogStream_nonBuffer()
// {
//     TFileLogger logger;
//     logger.setFileName("log/basicLogStream_nonBuffer.log");
//     QList<TLogger *> list;
//     list << &logger;
//     TBasicLogStream stream(list);
//     //stream.setNonBufferingMode();

//     TLog log(1, ba);

//     QBENCHMARK {
//         for (int i = 0; i < 10; ++i) {
//             stream.writeLog(log);

//             if ((i + 1) % 5 == 0)
//                 stream.flush();
//         }
//     }
//     stream.flush();
// }


void BenchMark::basicLogStream()
{
    TFileLogger logger;
    logger.setFileName("log/basicLogStream.log");
    QList<TLogger *> list;
    list << &logger;
    TBasicLogStream stream(list);

    TLog log(1, ba);

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            stream.writeLog(log);

            if ((i + 1) % 5 == 0)
                stream.flush();
        }
    }
    stream.flush();
}


void BenchMark::fileLogger()
{
    TFileLogger logger;
    logger.setFileName("log/fileLogger.log");
    logger.open();

    TLog log(1, ba);

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            logger.log(log);

            if ((i + 1) % 5 == 0)
                logger.flush();
        }
    }
}


void BenchMark::fileAioLogger()
{
    TFileAioLogger logger;
    logger.setFileName("log/fileAioLogger.log");
    logger.open();

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
    writer.setFileName("log/aioWriter.log");
    writer.open();

    QBENCHMARK {
        for (int i = 0; i < 10; ++i) {
            writer.write(ba.data(), ba.length());

            if ((i + 1) % 5 == 0)
                writer.flush();
        }
    }
}


void BenchMark::initTestCase()
{
    QDir logDir("log");
    logDir.mkpath(".");
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
