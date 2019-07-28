#include <QTest>
#include <QtCore>
#include <QDebug>
#include "tcache.h"

static qint64 FirstKey;
const int NUM = 500;


class TestCache : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void test();
    void insert_data();
    void insert();
    void bench_insert_binary();
    void bench_value_binary();
    void bench_insert_binary_lz4();
    void bench_value_binary_lz4();
    void bench_insert_text();
    void bench_value_text();
    void bench_insert_text_lz4();
    void bench_value_text_lz4();
};

static QByteArray genval(const QByteArray &key)
{
    QByteArray ret;

    auto d = QCryptographicHash::hash(key, QCryptographicHash::Sha3_512);
    int n = 5 * (d.mid(0,2).toHex().mid(0,1).toInt(nullptr, 16) + 1);
    ret.reserve(n * d.length());
    for (int i = 0; i < n; i++) {
        ret += d;
    }
    return ret;
}

void TestCache::initTestCase()
{
    TCache::setup();
    FirstKey = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void TestCache::cleanupTestCase()
{ }

void TestCache::test()
{
    TCache cache;
    QByteArray buf;
    cache.clear();

    QVERIFY(cache.count() == 0);
    QVERIFY(cache.value("hoge") == QByteArray());
    QVERIFY(cache.insert("hoge", "value", 1) == true);
    QVERIFY(cache.value("hoge") == "value");
    QVERIFY(cache.count() == 1);
    QVERIFY(cache.take("hoge") == "value");
    QVERIFY(cache.count() == 0);

    QVERIFY(cache.insert("hoge", "value", 1) == true);
    QVERIFY(cache.insert("hoge", "value2", 1) == true);
    QVERIFY(cache.value("hoge") == "value2");
    QVERIFY(cache.count() == 1);
    cache.remove("hoge");
    QVERIFY(cache.value("hoge") == QByteArray());
    QVERIFY(cache.count() == 0);

    QVERIFY(cache.insert("hoge", "value", 1) == true);
    QVERIFY(cache.count() == 1);
    Tf::msleep(1100);
    QVERIFY(cache.count() == 1);
    QVERIFY(cache.value("hoge") == QByteArray());
    cache.clear();
    QVERIFY(cache.count() == 0);

    for (int i = 0; i < 1000; i++) {
        QByteArray key = "foo";
        key += QByteArray::number(i);
        QVERIFY(cache.insert(key, QByteArray::number(Tf::rand32_r()), 5) == true);
    }
    QVERIFY(cache.count() == 1000);
    Tf::msleep(5000);
    QVERIFY(cache.value("foo1") == QByteArray());
    cache.clear();
    QVERIFY(cache.count() == 0);
}

void TestCache::insert_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("val");

    qint64 n = FirstKey;
    for (int i = 0; i < 20; i++) {
        QTest::newRow(QByteArray::number(i).data()) << QByteArray::number(n+i) << genval(QByteArray::number(n+i));
    }
}

void TestCache::insert()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, val);

    static TCache cache;

    cache.insert(key, val, 1000);
    QByteArray res = cache.value(key);
    QCOMPARE(res, val);
    qDebug() << "length of value: " << val.size();
}


void TestCache::bench_insert_binary()
{
    static TCache cache;
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.insert(QByteArray::number(FirstKey + i), genval(QByteArray::number(FirstKey + i)), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            cache.insert(QByteArray::number(r), genval(QByteArray::number(r)), 60);
        }
    }
}


void TestCache::bench_value_binary()
{
    static TCache cache;
    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = cache.value(QByteArray::number(r));
            Q_UNUSED(val);
        }
    }
}

void TestCache::bench_insert_binary_lz4()
{
    static TCache cache;
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.insert(QByteArray::number(FirstKey + i), Tf::lz4Compress(genval(QByteArray::number(FirstKey + i))), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto key = QByteArray::number(r);
            auto val = Tf::lz4Compress(genval(key));
            cache.insert(key, val, 60);
        }
    }
}


void TestCache::bench_value_binary_lz4()
{
    static TCache cache;
    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = Tf::lz4Uncompress(cache.value(QByteArray::number(r)));
            Q_UNUSED(val);
        }
    }
}


void TestCache::bench_insert_text()
{
    static TCache cache;
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.insert(QByteArray::number(FirstKey + i), genval(QByteArray::number(FirstKey + i)), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            cache.insert(QByteArray::number(r), genval(QByteArray::number(r).toHex()), 60);
        }
    }
}


void TestCache::bench_value_text()
{
    static TCache cache;
    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = cache.value(QByteArray::number(r));
            Q_UNUSED(val);
        }
    }
}

void TestCache::bench_insert_text_lz4()
{
    static TCache cache;
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.insert(QByteArray::number(FirstKey + i), Tf::lz4Compress(genval(QByteArray::number(FirstKey + i))), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto key = QByteArray::number(r);
            auto val = Tf::lz4Compress(genval(key).toHex());
            cache.insert(key, val, 60);
        }
    }
}


void TestCache::bench_value_text_lz4()
{
    static TCache cache;
    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = Tf::lz4Uncompress(cache.value(QByteArray::number(r)));
            Q_UNUSED(val);
        }
    }
}

QTEST_APPLESS_MAIN(TestCache)
#include "main.moc"
