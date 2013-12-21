#include <QTest>
#include <QtCore>
#include <QHostInfo>
#include <tglobal.h>
#include <stdio.h>


static QByteArray randomString()
{
    QByteArray data;
    data.reserve(128);

#if QT_VERSION >= 0x040700
    data.append(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
#else
    QDateTime now = QDateTime::currentDateTime();
    data.append(QByteArray::number(now.toTime_t()));
    data.append(QByteArray::number(now.time().msec()));
#endif
    data.append(QHostInfo::localHostName());
    data.append(QByteArray::number(QCoreApplication::applicationPid()));
    data.append(QByteArray::number((qulonglong)QThread::currentThread()));
    data.append(QByteArray::number((qulonglong)qApp));
    data.append(QByteArray::number(Tf::randXor128()));
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
}


static QByteArray randomString(int length)
{
    static char ch[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const int num = strlen(ch) - 1;
    QByteArray ret;
    ret.reserve(length);

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(num)];
    }
    return ret;
}



class Thread : public QThread
{
    void run()
    {
        for (;;) {
            for (int i = 0; i < 1000; ++i) {
                Tf::randXor128();
            }
            usleep(1);
        }
    }
};


class TestRand : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void random_data();
    void random();
    void bench_data();
    void bench();
    void randomstring1();
    void randomstring2();

private:
    enum {
        THREADS_NUM = 256,
    };

    Thread *thread[THREADS_NUM];
};


void TestRand::initTestCase()
{
    // put load for benchmarks
    for (int i = 0; i < THREADS_NUM; ++i) {
        thread[i] = new Thread;
        thread[i]->start();
    }
    Tf::msleep(100);
}


void TestRand::cleanupTestCase()
{
    for (int i = 0; i < THREADS_NUM; ++i) {
        thread[i]->terminate();
    }

    for (int i = 0; i < THREADS_NUM; ++i) {
        thread[i]->wait();
        delete thread[i];
    }
}


void TestRand::random_data()
{
    QTest::addColumn<int>("seed");
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 10;
    QTest::newRow("3") << 100;
}


void TestRand::random()
{
    QFETCH(int, seed);
    Tf::srandXor128(seed);
    for (int i = 0; i < 10; ++i)
        printf("rand: %u\n", Tf::randXor128());
}


void TestRand::bench_data()
{
    Tf::srandXor128(1222);
}


void TestRand::bench()
{
    QBENCHMARK {
        Tf::randXor128();
     }
}

void TestRand::randomstring1()
{
    QBENCHMARK {
        randomString();
    }
}

void TestRand::randomstring2()
{
    QBENCHMARK {
        randomString(20);
    }
}

QTEST_APPLESS_MAIN(TestRand)
#include "main.moc"

/*
rand: 3656013425
rand: 503675675
rand: 4013738704
rand: 4013743934
rand: 1693336950
rand: 1359361875
rand: 1483021801
rand: 1370094836
rand: 1199228482
rand: 665247057
rand: 3656013434
rand: 503675664
rand: 4013738715
rand: 4013721446
rand: 1693336957
rand: 1359376139
rand: 1483021794
rand: 1399432767
rand: 1169867906
rand: 665224457
rand: 3656013332
rand: 503675774
rand: 4013738677
rand: 4013945878
rand: 1693336851
rand: 1359289467
rand: 1483021708
rand: 1223347666
rand: 1580916481
rand: 665187961
*/
