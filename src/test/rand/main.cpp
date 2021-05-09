#include <QTest>
#include <QtCore>
#include <QHostInfo>
#if QT_VERSION >= 0x050a00
#include <QRandomGenerator>
#endif
#include <tglobal.h>
#include <stdio.h>
#include <random>

static QMutex mutex;


static QByteArray randomString()
{
    QByteArray data;
    data.reserve(128);

    data.append(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
    data.append(QHostInfo::localHostName().toLatin1());
    data.append(QByteArray::number(QCoreApplication::applicationPid()));
    data.append(QByteArray::number((qulonglong)QThread::currentThread()));
    data.append(QByteArray::number((qulonglong)qApp));
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


class TestRand : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void tf_rand_r();
    void random_device();
    void mt19937();
    void mt19937_64();
    void mt19937_with_uniform();
    void minstd_rand();
    void ranlux24_base();
    void ranlux48_base();
#if QT_VERSION >= 0x050a00
    void rand_QRandomGenerator_global();
    void rand_QRandomGenerator_system();
#endif
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
};


void TestRand::initTestCase()
{
}

void TestRand::cleanupTestCase()
{
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

#if QT_VERSION >= 0x050a00
void TestRand::rand_QRandomGenerator_global()
{
    QBENCHMARK {
        mutex.lock();
        QRandomGenerator::global()->generate();
        mutex.unlock();
    }
}


void TestRand::rand_QRandomGenerator_system()
{
    QBENCHMARK {
        mutex.lock();
        QRandomGenerator::system()->generate();
        mutex.unlock();
    }
}
#endif

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
