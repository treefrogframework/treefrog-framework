#include <QTest>
#include <QtCore>
#include <tglobal.h>


class TestDateTime : public QObject
{
    Q_OBJECT
private slots:
    void compare();
    void benchQtCurrentDateTime();
    void benchTfCurrentDateTime();
};


void TestDateTime::compare()
{
    for (int i = 0; i < 3; ++i) {
        QDateTime qt = QDateTime::currentDateTime();
        QDateTime tf = Tf::currentDateTimeSec();
        QCOMPARE(qt.date().year(),   tf.date().year());
        QCOMPARE(qt.date().month(),  tf.date().month());
        QCOMPARE(qt.date().day(),    tf.date().day());
        QCOMPARE(qt.time().hour(),   tf.time().hour());
        QCOMPARE(qt.time().minute(), tf.time().minute());
        QCOMPARE(qt.time().second(), tf.time().second());

        Tf::msleep(1500);
    }
}


void TestDateTime::benchQtCurrentDateTime()
{
    QBENCHMARK {
        QDateTime dt = QDateTime::currentDateTime();
    }
}


void TestDateTime::benchTfCurrentDateTime()
{
    QBENCHMARK {
        QDateTime dt = Tf::currentDateTimeSec();
    }
}

QTEST_APPLESS_MAIN(TestDateTime)
#include "main.moc"
