#include <QTest>
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
    void date();
    void parse();
};


void TestMailMessage::mimeEncode_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QByteArray>("result");
    QTest::newRow("1") << QString::fromUtf8("無事？") << QByteArray("=?ISO-2022-JP?B?GyRCTDU7diEpGyhC?=");
}


void TestMailMessage::mimeEncode()
{
    QFETCH(QString, data);
    QFETCH(QByteArray, result);
    QByteArray actl = THttpUtility::toMimeEncoded(data, "ISO-2022-JP");
//     qDebug("%s", result.data());
//     qDebug("%s", actl.data());
    QCOMPARE(result, actl);
}


void TestMailMessage::mimeDecode_data()
{
    QTest::addColumn<QString>("data");
    QTest::addColumn<QByteArray>("encoding");

    QTest::newRow("1") << QString::fromUtf8("aaaa") << QByteArray("UTF-8");
    QTest::newRow("2") << QString::fromUtf8("あいうえお") << QByteArray("UTF-8");
    QTest::newRow("3") << QString::fromUtf8("あいうえお") << QByteArray("EUC-JP");
    QTest::newRow("4") << QString::fromUtf8("０0１1２2３3４4５5６6７7８8９9") << QByteArray("UTF-8");
    QTest::newRow("5") << QString::fromUtf8("あaaa") << QByteArray("iso-2022-jp");
    QTest::newRow("6") << QString::fromUtf8("無事？") << QByteArray("UTF-8");
    QTest::newRow("7") << QString::fromUtf8("無事？") << QByteArray("shift-jis");
 
}


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

    QTest::newRow("1") << QString::fromUtf8("無事？") << QByteArray("UTF-8");
    QTest::newRow("2") << QString::fromUtf8("こんにちは") << QByteArray("ISO-2022-JP");
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
    
    QTest::newRow("1") << QByteArray("iso-2022-jp")
                       << QByteArray("aol1@aol.com")
                       << QByteArray("aol2@aol.com")
                       << QByteArray("aol3@aol.com")
                       << QByteArray("aol4@aol.com")
                       << QString::fromUtf8("無事？")
                       << QByteArray("=?ISO-2022-JP?B?GyRCTDU7diEpGyhC?= <aol1@aol.com>");

    QTest::newRow("2") << QByteArray("UTF-8")
                       << QByteArray("aol1@aol.com")
                       << QByteArray("aol2@aol.com")
                       << QByteArray("aol3@aol.com")
                       << QByteArray("aol4@aol.com")
                       << QString::fromUtf8("aoyama kazz")
                       << QByteArray("aoyama kazz <aol1@aol.com>");
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
    qDebug("%s", msg.from().data());
    QCOMPARE(msg.from(), result);
    QCOMPARE(msg.fromAddress(), from);

    msg.addTo(to, name);
    msg.addCc(cc, name);
    msg.addBcc(bcc, name);
    QList<QByteArray> lst;
    lst << "aol2@aol.com" << "aol3@aol.com" << "aol4@aol.com";
    QCOMPARE(msg.recipients(), lst);
    qDebug("%s", msg.toByteArray().data());
}


void TestMailMessage::date()
{
    TMailMessage msg;
    msg.setDate(QDateTime(QDate(2011,3,28), QTime(12,11,04)));
    qDebug("%s", msg.date().data());
    QCOMPARE(msg.date(), QByteArray("Mon, 28 Mar 2011 12:11:04 +0900"));
}


void TestMailMessage::parse()
{
    QString msg = QString::fromUtf8(
        "From: hoge <test@example.com>\n"               \
        "To: <test1@example.jp>\n"                      \
        "Cc: aoyama <test3@example.jp>\n"               \
        "Bcc: <test3@example.jp>, <test4@example.jp>\n" \
        "Date: Wed, 30 Mar 2011 19:53:04 +0900\n"       \
        "\n"                                            \
        "こんにちは");

    TMailMessage mail(msg);
    qDebug("%s", mail.toByteArray().data());
    qDebug("%d", mail.recipients().count());
    foreach(QByteArray ba, mail.recipients()) {
        qDebug("recpt: %s", ba.data());
    }
    QCOMPARE(mail.recipients().count(), 3);
}

QTEST_MAIN(TestMailMessage)
#include "main.moc"
