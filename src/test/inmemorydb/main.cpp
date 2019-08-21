#include <TfTest/TfTest>
#include <QtCore>
#include <QDebug>
#include "tcacheinmemorystore.h"

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
    FirstKey = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void TestCache::cleanupTestCase()
{ }

void TestCache::test()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();
    QByteArray buf;
    cache.clear();

    QVERIFY(cache.get("hoge") == QByteArray());
    QVERIFY(cache.set("hoge", "value", 1000) == true);
    QVERIFY(cache.get("hoge") == "value");

    QVERIFY(cache.set("hoge", "value", 1000) == true);
    QVERIFY(cache.set("hoge", "value2", 1000) == true);
    QVERIFY(cache.get("hoge") == "value2");
    cache.remove("hoge");
    QVERIFY(cache.get("hoge") == QByteArray());

    QVERIFY(cache.set("hoge", "value", 1000) == true);
    Tf::msleep(1100);
    QVERIFY(cache.get("hoge") == QByteArray());
    cache.clear();

    for (int i = 0; i < 1000; i++) {
        QByteArray key = "foo";
        key += QByteArray::number(i);
        QVERIFY(cache.set(key, QByteArray::number(Tf::rand32_r()), 5000) == true);
    }
    QVERIFY(! cache.get("foo1").isEmpty());
    Tf::msleep(5000);
    QVERIFY(cache.get("foo1") == QByteArray());
    cache.clear();

    cache.close();
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

    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();

    cache.set(key, val, 1000);
    QByteArray res = cache.get(key);
    QCOMPARE(res, val);
    qDebug() << "length of value: " << val.size();
    cache.close();
}


void TestCache::bench_insert_binary()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.set(QByteArray::number(FirstKey + i), genval(QByteArray::number(FirstKey + i)), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            cache.set(QByteArray::number(r), genval(QByteArray::number(r)), 60);
        }
    }
    cache.close();
}


void TestCache::bench_value_binary()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = cache.get(QByteArray::number(r));
            Q_UNUSED(val);
        }
    }
    cache.close();
}

void TestCache::bench_insert_binary_lz4()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.set(QByteArray::number(FirstKey + i), Tf::lz4Compress(genval(QByteArray::number(FirstKey + i))), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto key = QByteArray::number(r);
            auto val = (genval(key));
            cache.set(key, val, 60);
        }
    }
    cache.close();
}


void TestCache::bench_value_binary_lz4()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = Tf::lz4Uncompress(cache.get(QByteArray::number(r)));
            Q_UNUSED(val);
        }
    }
    cache.close();
}


void TestCache::bench_insert_text()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.set(QByteArray::number(FirstKey + i), genval(QByteArray::number(FirstKey + i)), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            cache.set(QByteArray::number(r), genval(QByteArray::number(r).toHex()), 60);
        }
    }
    cache.close();
}


void TestCache::bench_value_text()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = cache.get(QByteArray::number(r));
            Q_UNUSED(val);
        }
    }
}

void TestCache::bench_insert_text_lz4()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();
    cache.clear();

    for (int i = 0; i < 200; i++) {
        cache.set(QByteArray::number(FirstKey + i), Tf::lz4Compress(genval(QByteArray::number(FirstKey + i))), 60);
    }

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto key = QByteArray::number(r);
            auto val = (genval(key).toHex());
            cache.set(key, val, 60);
        }
    }
}


void TestCache::bench_value_text_lz4()
{
    TCacheInMemoryStore cache = TCacheInMemoryStore();
    cache.open();

    QBENCHMARK {
        for (int i = 0; i < 100; i++) {
            int r = Tf::random(FirstKey, FirstKey + NUM - 1);
            auto val = Tf::lz4Uncompress(cache.get(QByteArray::number(r)));
            Q_UNUSED(val);
        }
    }
}

TF_TEST_SQLLESS_MAIN(TestCache)
#include "main.moc"
