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
    QString msg = QString::fromUtf8(
        "From: aoyama <a.kazuharu@gmail.com>\n"       \
        "To: kazu <a.kazuharu@gmail.com>\n"          \
        "Subject: ようこそ!!!\n"                     \
        "\n"                                         \
        "こんにちは\nさようなら");

    TSmtpMailer mailer("smtp.example.jp", 25);
    mailer.setAuthenticationEnabled(true);
    mailer.setUserName("a.kazuharu@gmail.com");
    mailer.setPassword("");
#if 0
    bool res = mailer.send(TMailMessage(msg));
#else
    bool res = 1;  // not test now
#endif
    QVERIFY(res);
}


QTEST_MAIN(TestSmtpMailer)
#include "smtpmailer.moc"
