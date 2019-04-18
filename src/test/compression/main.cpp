#include <QTest>
#include <QDebug>
#include "tglobal.h"

static QByteArray dummy;

class LZ4Compress : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void qcompress_data();
    void qcompress();
    void lz4_data();
    void lz4();
    void bench_lz4();
    void bench_qcompress();
    void bench_base64();
};


void LZ4Compress::initTestCase()
{
    dummy.resize(4 * 1024 * 1024);
    for (int i = 0; i < dummy.size() - 10; i += 10) {
        dummy.replace(i, 4, QByteArray::number((uint)Tf::random(0xFFFF), 16));
    }
}

void LZ4Compress::qcompress_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << QByteArray("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    QTest::newRow("3") << QByteArray("a;lkdfj;lkjfad;lakjsdla;kj;lkajd;lakj;lkj;lkj;sl;kjs;lkj;llkj;lslkj;lslkj;slkjsl;kj;lkj;lkjkj;lkk");
    QTest::newRow("4") << QByteArray("109283091823019823019283019823019823019283019831092830192830192831");
    QTest::newRow("5") << QByteArray("poiterpoiterpoiterpoiterpoierteeprotipoeritepoirtperopoitepotierpoeritperoitpeorit");
    QTest::newRow("6") << dummy;
}


void LZ4Compress::qcompress()
{
    QFETCH(QByteArray, data);

    QByteArray comp = qCompress(data);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = qUncompress(comp);
    QCOMPARE(data, uncomp);
}


void LZ4Compress::lz4_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << QByteArray("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    QTest::newRow("3") << QByteArray("a;lkdfj;lkjfad;lakjsdla;kj;lkajd;lakj;lkj;lkj;sl;kjs;lkj;llkj;lslkj;lslkj;slkjsl;kj;lkj;lkjkj;lkk");
    QTest::newRow("4") << QByteArray("109283091823019823019283019823019823019283019831092830192830192831");
    QTest::newRow("5") << QByteArray("poiterpoiterpoiterpoiterpoierteeprotipoeritepoirtperopoitepotierpoeritperoitpeorit");
    QTest::newRow("6") << dummy;
}


void LZ4Compress::lz4()
{
    QFETCH(QByteArray, data);

    QByteArray comp = Tf::lz4Compress(data);
    qDebug() << "orignal length:" << data.length() <<  " compression:" << (float)comp.length() / qMax(data.length(), 1);
    QByteArray uncomp = Tf::lz4Uncompress(comp);
    QCOMPARE(data, uncomp);
}


void LZ4Compress::bench_lz4()
{
    dummy.resize(512 * 1024);

    QBENCHMARK {
        auto cmp = Tf::lz4Uncompress( Tf::lz4Compress(dummy) );
        Q_UNUSED(cmp);
    }
}


void LZ4Compress::bench_qcompress()
{
    dummy.resize(512 * 1024);

    QBENCHMARK {
        auto cmp = qUncompress( qCompress(dummy) );
        Q_UNUSED(cmp);
    }
}


void LZ4Compress::bench_base64()
{
    dummy.resize(512 * 1024);

    QBENCHMARK {
        auto cmp = QByteArray::fromBase64( dummy.toBase64() );
        Q_UNUSED(cmp);
    }
}

QTEST_APPLESS_MAIN(LZ4Compress)
#include "main.moc"
