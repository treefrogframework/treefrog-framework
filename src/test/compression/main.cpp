#include <QTest>
#include <QDebug>
#include "tglobal.h"

static QByteArray dummydata;
static const QByteArray testdata2("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
static const QByteArray testdata3("a;lkdfj;lkjfad;lakjsdla;kj;lkajd;lakj;lkj;lkj;sl;kjs;lkj;llkj;lslkj;lslkj;slkjsl;kj;lkj;lkjkj;lkk");
static const QByteArray testdata4("109283091823019823019283019823019823019283019831092830192830192831");
static const QByteArray testdata5("poiterpoiterpoiterpoiterpoierteeprotipoeritepoirtperopoitepotierpoeritperoitpeorit");


class LZ4Compress : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void qcompress_data();
    void qcompress();
    void lz4_l1_data();
    void lz4_l1();
    void lz4_l2_data();
    void lz4_l2();
    void lz4_l5_data();
    void lz4_l5();
    void bench_lz4_l1_512();
    void bench_lz4_l2_512();
    void bench_lz4_l5_512();
    void bench_qcompress_512();
    void bench_base64_512();
    void bench_lz4_l1_128k();
    void bench_lz4_l2_128k();
    void bench_lz4_l5_128k();
    void bench_qcompress_128k();
    void bench_base64_128k();
    void bench_lz4_l1_10m();
    void bench_lz4_l2_10m();
    void bench_lz4_l5_10m();
    void bench_qcompress_10m();
    void bench_base64_10m();
};


void LZ4Compress::initTestCase()
{
    dummydata.resize(10 * 1024 * 1024);
    for (int i = 0; i < dummydata.size() - 8; i += 8) {
        dummydata.replace(i, 4, QByteArray::number((uint)Tf::random(0xFFFF), 16));
    }
}

void LZ4Compress::qcompress_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << testdata2;
    QTest::newRow("3") << testdata3;
    QTest::newRow("4") << testdata4;
    QTest::newRow("5") << testdata5;
    QTest::newRow("6") << dummydata.mid(0, 1025);
    QTest::newRow("7") << dummydata.mid(0, 1024 * 1021);
    QTest::newRow("8") << dummydata;
}


void LZ4Compress::qcompress()
{
    QFETCH(QByteArray, data);

    QByteArray comp = qCompress(data);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = qUncompress(comp);
    QCOMPARE(data, uncomp);
}


void LZ4Compress::lz4_l1_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << testdata2;
    QTest::newRow("3") << testdata3;
    QTest::newRow("4") << testdata4;
    QTest::newRow("5") << testdata5;
    QTest::newRow("6") << dummydata.mid(0, 1025);
    QTest::newRow("7") << dummydata.mid(0, 1024 * 1021);
    QTest::newRow("8") << dummydata;
}


void LZ4Compress::lz4_l1()
{
    QFETCH(QByteArray, data);

    QByteArray comp = Tf::lz4Compress(data, 1);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = Tf::lz4Uncompress(comp);
    QCOMPARE(data, uncomp);
}


void LZ4Compress::lz4_l2_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << testdata2;
    QTest::newRow("3") << testdata3;
    QTest::newRow("4") << testdata4;
    QTest::newRow("5") << testdata5;
    QTest::newRow("6") << dummydata.mid(0, 1025);
    QTest::newRow("7") << dummydata.mid(0, 1024 * 1021);
    QTest::newRow("8") << dummydata;
}


void LZ4Compress::lz4_l2()
{
    QFETCH(QByteArray, data);

    QByteArray comp = Tf::lz4Compress(data, 2);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = Tf::lz4Uncompress(comp);
    QCOMPARE(data, uncomp);
}

void LZ4Compress::lz4_l5_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << testdata2;
    QTest::newRow("3") << testdata3;
    QTest::newRow("4") << testdata4;
    QTest::newRow("5") << testdata5;
    QTest::newRow("6") << dummydata.mid(0, 1025);
    QTest::newRow("7") << dummydata.mid(0, 1024 * 1021);
    QTest::newRow("8") << dummydata;
}


void LZ4Compress::lz4_l5()
{
    QFETCH(QByteArray, data);

    QByteArray comp = Tf::lz4Compress(data, 5);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = Tf::lz4Uncompress(comp);
    QCOMPARE(data, uncomp);
}

void LZ4Compress::bench_lz4_l1_512()
{
    auto d = dummydata.mid(0, 512);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 1));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l2_512()
{
    auto d = dummydata.mid(0, 512);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 2));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l5_512()
{
    auto d = dummydata.mid(0, 512);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 5));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_qcompress_512()
{
    auto d = dummydata.mid(0, 512);
    QBENCHMARK {
        auto cmp = qUncompress(qCompress(d));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_base64_512()
{
    auto d = dummydata.mid(0, 512);
    QBENCHMARK {
        auto cmp = QByteArray::fromBase64(d.toBase64());
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l1_128k()
{
    auto d = dummydata.mid(0, 128 * 1024);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 1));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l2_128k()
{
    auto d = dummydata.mid(0, 128 * 1024);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 2));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l5_128k()
{
    auto d = dummydata.mid(0, 128 * 1024);
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(d, 5));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_qcompress_128k()
{
    auto d = dummydata.mid(0, 128 * 1024);
    QBENCHMARK {
        auto cmp = qUncompress(qCompress(d));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_base64_128k()
{
    auto d = dummydata.mid(0, 128 * 1024);
    QBENCHMARK {
        auto cmp = QByteArray::fromBase64(d.toBase64());
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l1_10m()
{
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(dummydata, 1));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l2_10m()
{
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(dummydata, 2));
        Q_UNUSED(cmp);
    }
}

void LZ4Compress::bench_lz4_l5_10m()
{
    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress(Tf::lz4Compress(dummydata, 5));
        Q_UNUSED(cmp);
    }
}


void LZ4Compress::bench_qcompress_10m()
{
    QBENCHMARK {
        auto cmp = qUncompress(qCompress(dummydata));
        Q_UNUSED(cmp);
    }
}


void LZ4Compress::bench_base64_10m()
{
    QBENCHMARK {
        auto cmp = QByteArray::fromBase64(dummydata.toBase64());
        Q_UNUSED(cmp);
    }
}

QTEST_APPLESS_MAIN(LZ4Compress)
#include "main.moc"
