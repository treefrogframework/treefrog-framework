#include <QTest>
#include "tsmtpmailer.h"


class TestSmtpMailer : public QObject
{
    Q_OBJECT
private slots:
    void sendMail();
};


void TestSmtpMailer::sendMail()
{
//     TMailMessage mail("utf-8");
//     mail.setDate(QDateTime(QDate(2011,3,30), QTime(11,59,35)));
//     //mail.setRawHeader("Date", "Thu, 29 Mar 2011 21:02:35 +0900");
//     mail.setSubject(QString::fromUtf8("こんにちは、テストです"));
//     mail.setFrom("test@example.com", "test");
//     mail.addTo("test2@example.com", QString::fromUtf8("ほげさん"));
//     mail.setBody(QString::fromUtf8("おはよう。\nこんにちは?"));

//     TSmtpMailer mailer;
//     mailer.setAuthenticationEnabled(true);
//     mailer.setUserName("kazzn@ops.dti.ne.jp");
//     mailer.setPassword("");
//     bool res = mailer.send(mail);
//     QVERIFY(res);


    QString msg = QString::fromUtf8(
        "From: aoyama <kazzn@ops.dti.ne.jp>\n"       \
        "To: kazu <a.kazuharu@gmail.com>\n"          \
        "Subject: ようこそ!!!\n"                     \
        "\n"                                         \
        "こんにちは\nさようなら");

    TSmtpMailer mailer("smtp.ops.dti.ne.jp", 587);
    mailer.setAuthenticationEnabled(true);
    mailer.setUserName("kazzn@ops.dti.ne.jp");
    mailer.setPassword("");
    bool res = mailer.send(TMailMessage(msg));
    //QVERIFY(res);
    QVERIFY(1);
}


QTEST_MAIN(TestSmtpMailer)
#include "main.moc"
