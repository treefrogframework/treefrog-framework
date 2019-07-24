#include <QTest>
#include <QDebug>
#include "tcache.h"

static qint64 FirstKey;


class Cache : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void insert_data();
    void insert();
    void bench_cache();
};

static QByteArray genval(const QByteArray &key)
{
    QByteArray ret;
    auto d = QCryptographicHash::hash(key, QCryptographicHash::Md5);
    int n = d.mid(0,2).toHex().mid(0,1).toInt(nullptr, 16) + 1;
    for (int i = 0; i < n; i++) {
        ret += d;
    }
    return ret;
}

void Cache::initTestCase()
{
    TCache::setup();
    FirstKey = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void Cache::insert_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("val");

    qint64 n = FirstKey;
    QTest::newRow("1") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("2") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("3") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("4") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("5") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("6") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("7") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("8") << QByteArray::number(n) << genval(QByteArray::number(n));
    ++n;
    QTest::newRow("9") << QByteArray::number(n) << genval(QByteArray::number(n));
}

void Cache::insert()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, val);

    static TCache cache(1024*1024);

    cache.insert(key, val, 1000);
    QByteArray res = cache.value(key);
    QCOMPARE(res, val);
    qDebug() << "val: " << val.toHex();
}


void Cache::bench_cache()
{
    // auto d = dummydata.mid(0, 512);
    // QBENCHMARK {
    //     auto cmp = QByteArray::fromBase64(d.toBase64());
    //     Q_UNUSED(cmp);
    // }
}

QTEST_APPLESS_MAIN(Cache)
#include "main.moc"
