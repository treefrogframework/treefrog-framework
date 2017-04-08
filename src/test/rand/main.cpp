#include <QTest>
#include <QtCore>
#include <QHostInfo>
#include <tglobal.h>
#include <stdio.h>
#include <random>

static QMutex mutex;


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
        ret += ch[Tf::random(0, num)];
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
    void tf_randXor128_data();
    void tf_randXor128();
    void tf_randXor128_bench_data();
    void tf_randXor128_bench();
    void tf_rand_r();
    void random_device();
    void mt19937();
    void mt19937_64();
    void mt19937_with_uniform();
    void minstd_rand();
    void ranlux24_base();
    void ranlux48_base();
    void randomstring1();
    void randomstring2();

private:
    enum {
#ifdef Q_OS_LINUX
        THREADS_NUM = 256,
#else
        THREADS_NUM = 64,
#endif
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


void TestRand::tf_randXor128_data()
{
    QTest::addColumn<int>("seed");
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 10;
    QTest::newRow("3") << 100;
}


void TestRand::tf_randXor128()
{
    QFETCH(int, seed);
    Tf::srandXor128(seed);
    for (int i = 0; i < 10; ++i)
        printf("rand: %u\n", Tf::randXor128());
}


void TestRand::tf_randXor128_bench_data()
{
    Tf::srandXor128(1222);
}


void TestRand::tf_randXor128_bench()
{
    QBENCHMARK {
        Tf::randXor128();
     }
}

void TestRand::tf_rand_r()
{
    QBENCHMARK {
        Tf::rand32_r();
    }
}

void TestRand::random_device()
{
    std::random_device rd;
    QBENCHMARK {
        mutex.lock();
        rd();
        mutex.unlock();
    }
}

// Mersenne Twister 32bit
void TestRand::mt19937()
{
    std::mt19937 mt(1);
    QBENCHMARK {
        mutex.lock();
        mt();
        mutex.unlock();
    }
}

// Mersenne Twister 64bit
void TestRand::mt19937_64()
{
    std::mt19937_64 mt(1);
    QBENCHMARK {
        mutex.lock();
        mt();
        mutex.unlock();
    }
}

// Mersenne Twister / uniform_int_distribution
void TestRand::mt19937_with_uniform()
{
    std::mt19937 mt(1);
    std::uniform_int_distribution<int> form(0, 65536);
    QBENCHMARK {
        mutex.lock();
        form(mt);
        mutex.unlock();
    }
}

// Linear congruential generator
void TestRand::minstd_rand()
{
    std::minstd_rand mr(1);
    QBENCHMARK {
        mr();
    }
}

// Lagged Fibonacci 24bit
void TestRand::ranlux24_base()
{
    std::ranlux24_base rb(1);
    QBENCHMARK {
        mutex.lock();
        rb();
        mutex.unlock();
    }
}

// Lagged Fibonacci 48bit
void TestRand::ranlux48_base()
{
    std::ranlux48_base rb(1);
    QBENCHMARK {
        mutex.lock();
        rb();
        mutex.unlock();
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
