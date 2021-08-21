#include <TfTest/TfTest>
#include <THttpUtility>
#include "tmailmessage.h"


class TestMailMessage : public QObject
{
    Q_OBJECT
private slots:
    void mimeEncode_data();
    void mimeEncode();
    void mimeDecode_data();
    void mimeDecode();
    void subject_data();
    void subject();
    void addAddress_data();
    void addAddress();
    void dateTime_data();
    void dateTime();
    void parse();
};


void TestMailMessage::mimeEncode_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QByteArray>("result");

    // `echo 無事？ | nkf -jM`
    QTest::newRow("1") << QString::fromUtf8(u8"無事？") << QByteArray(u8"=?ISO-2022-JP?B?GyRCTDU7diEpGyhC?=");
    QTest::newRow("2") << QString::fromUtf8(u8"田") << QByteArray(u8"=?ISO-2022-JP?B?GyRCRUQbKEI=?=");
    QTest::newRow("3") << QString::fromUtf8(u8"あ1１aAＡい2２漢字3") << QByteArray(u8"=?ISO-2022-JP?B?GyRCJCIbKEIxGyRCIzEbKEJhQRskQiNBJCQbKEIyGyRCIzI0QTt6GyhCMw==?=");
    QTest::newRow("4") << QString::fromUtf8(u8"1１aAＡい2２漢字3") << QByteArray(u8"=?ISO-2022-JP?B?MRskQiMxGyhCYUEbJEIjQSQkGyhCMhskQiMyNEE7ehsoQjM=?=");
}


void TestMailMessage::mimeEncode()
{
    QFETCH(QString, data);
    QFETCH(QByteArray, result);
    QByteArray actl = THttpUtility::toMimeEncoded(data, "ISO-2022-JP");
    // qDebug("%s", result.data());
    // qDebug("%s", actl.data());
    QCOMPARE(result, actl);
}


void TestMailMessage::mimeDecode_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QByteArray>("encoding");

    QTest::newRow("1") << QString::fromUtf8(u8"aaaa") << QByteArray(u8"UTF-8");
    QTest::newRow("2") << QString::fromUtf8(u8"あいうえお") << QByteArray(u8"UTF-8");
    QTest::newRow("3") << QString::fromUtf8(u8"あいうえお") << QByteArray(u8"EUC-JP");
    QTest::newRow("4") << QString::fromUtf8(u8"０0１1２2３3４4５5６6７7８8９9") << QByteArray(u8"UTF-8");
    QTest::newRow("5") << QString::fromUtf8(u8"あaaa") << QByteArray(u8"iso-2022-jp");
    QTest::newRow("6") << QString::fromUtf8(u8"無事？") << QByteArray(u8"UTF-8");
    QTest::newRow("7") << QString::fromUtf8(u8"無事？") << QByteArray(u8"shift-jis");
    QTest::newRow("8") << QString::fromUtf8(u8"無a事？z") << QByteArray(u8"iso-2022-jp");
    QTest::newRow("9") << QString::fromUtf8(u8"無a事？z") << QByteArray(u8"UTF-8");
    QTest::newRow("10") << QString::fromUtf8(u8"無a事？z") << QByteArray(u8"shift-jis");}


void TestMailMessage::mimeDecode()
{
    QFETCH(QString, data);
    QFETCH(QByteArray, encoding);

    QString result = THttpUtility::fromMimeEncoded(THttpUtility::toMimeEncoded(data, encoding).data());
    //qDebug("%s", THttpUtility::toMimeEncoded(data, encoding).data());
    QCOMPARE(data, result);
}

void TestMailMessage::subject_data()
{
    QTest::addColumn<QString>("subject");
    QTest::addColumn<QByteArray>("encoding");

    QTest::newRow("1") << QString::fromUtf8(u8"無事？") << QByteArray(u8"UTF-8");
    QTest::newRow("2") << QString::fromUtf8(u8"こんにちは") << QByteArray(u8"ISO-2022-JP");
}


void TestMailMessage::subject()
{
    QFETCH(QString, subject);
    QFETCH(QByteArray, encoding);

    TMailMessage msg(encoding);
    msg.setSubject(subject);
    //qDebug("%s", msg.toByteArray().data());
    QCOMPARE(subject, msg.subject());
}


void TestMailMessage::addAddress_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("from");
    QTest::addColumn<QByteArray>("to");
    QTest::addColumn<QByteArray>("cc");
    QTest::addColumn<QByteArray>("bcc");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray(u8"iso-2022-jp")
                       << QByteArray(u8"aol1@aol.com")
                       << QByteArray(u8"aol2@aol.com")
                       << QByteArray(u8"aol3@aol.com")
                       << QByteArray(u8"aol4@aol.com")
                       << QString::fromUtf8(u8"無事？")
                       << QByteArray(u8"=?ISO-2022-JP?B?GyRCTDU7diEpGyhC?= <aol1@aol.com>");

    QTest::newRow("2") << QByteArray(u8"UTF-8")
                       << QByteArray(u8"aol1@aol.com")
                       << QByteArray(u8"aol2@aol.com")
                       << QByteArray(u8"aol3@aol.com")
                       << QByteArray(u8"aol4@aol.com")
                       << QString::fromUtf8(u8"aoyama kazz")
                       << QByteArray(u8"aoyama kazz <aol1@aol.com>");
}

void TestMailMessage::addAddress()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QByteArray, from);
    QFETCH(QByteArray, to);
    QFETCH(QByteArray, cc);
    QFETCH(QByteArray, bcc);
    QFETCH(QString, name);
    QFETCH(QByteArray, result);

    TMailMessage msg(encoding);
    msg.setFrom(from, name);
    // qDebug("from: %s", msg.from().data());
    // qDebug("expt: %s", result.data());
    QCOMPARE(msg.from(), result);
    QCOMPARE(msg.fromAddress(), from);

    msg.addTo(to, name);
    msg.addCc(cc, name);
    msg.addBcc(bcc, name);
    QByteArrayList lst;
    lst << "aol2@aol.com" << "aol3@aol.com" << "aol4@aol.com";
    QCOMPARE(msg.recipients(), lst);
    //qDebug("%s", msg.toByteArray().data());
}

void TestMailMessage::dateTime_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QString>("result");

    // Timezone
    uint utc = QDateTime(QDate(2000,1,1), QTime(0,0,0), Qt::UTC).toSecsSinceEpoch();
    uint local = QDateTime(QDate(2000,1,1), QTime(0,0,0), Qt::LocalTime).toSecsSinceEpoch();
    int offset = (utc - local) / 60;
    QString offsetStr = QString("%1%2%3")
        .arg(offset > 0 ? '+' : '-')
        .arg(qAbs(offset) / 60, 2, 10, QLatin1Char('0'))
        .arg(qAbs(offset) % 60, 2, 10, QLatin1Char('0'));

    QTest::newRow("1") << QDateTime(QDate(2011,3,28), QTime(12,11,04), Qt::LocalTime) << "Mon, 28 Mar 2011 12:11:04 " + offsetStr;
    QTest::newRow("2") << QDateTime(QDate(2014,3,31), QTime( 1, 0, 0), Qt::LocalTime) << "Mon, 31 Mar 2014 01:00:00 " + offsetStr;
    QTest::newRow("3") << QDateTime(QDate(2011,3,28), QTime(12,11,04), Qt::UTC)       << "Mon, 28 Mar 2011 12:11:04 +0000";
    QTest::newRow("4") << QDateTime(QDate(2014,3,31), QTime( 1, 0, 0), Qt::UTC)       << "Mon, 31 Mar 2014 01:00:00 +0000";
}

void TestMailMessage::dateTime()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(QString, result);

    TMailMessage msg;
    msg.setDate(dateTime);

    QCOMPARE(msg.date(), result.toLatin1());
}

void TestMailMessage::parse()
{
    QString msg = QString::fromUtf8(
      u8"From: hoge <test@example.com>\n"               \
        "To: <test1@example.jp>\n"                      \
        "Cc: aoyama <test3@example.jp>\n"               \
        "Bcc: <test3@example.jp>, <test4@example.jp>\n" \
        "Date: Wed, 30 Mar 2011 19:53:04 +0900\n"       \
        "\n"                                            \
        "こんにちは,世界");

    TMailMessage mail(msg);
    //qDebug("%s", mail.toByteArray().data());
    //qDebug("%d", mail.recipients().count());
    // foreach(QByteArray ba, mail.recipients()) {
    //     qDebug("recpt: %s", ba.data());
    // }
    QCOMPARE(mail.recipients().count(), 3);
    QCOMPARE(mail.from(), QByteArray("hoge <test@example.com>"));
    QCOMPARE(mail.to(), QByteArray("<test1@example.jp>"));
    QCOMPARE(mail.cc(), QByteArray("aoyama <test3@example.jp>"));
    QCOMPARE(mail.bcc(), QByteArray("<test3@example.jp>, <test4@example.jp>"));
    QCOMPARE(mail.body(), QString::fromUtf8("こんにちは,世界"));
}

TF_TEST_MAIN(TestMailMessage)
#include "main.moc"
