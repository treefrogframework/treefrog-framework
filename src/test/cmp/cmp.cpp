#include <TfTest/TfTest>
#include <cstring>
#include <tglobal.h>

constexpr size_t LEN = 2048;
QByteArray str1;
QByteArray str2;
QByteArray str3;
QString qstr1;
QString qstr2;
QString qstr3;


static QByteArray randomString(int length)
{
    static char ch[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const int num = strlen(ch) - 1;
    QByteArray ret;
    ret.reserve(length);

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(0, num)];
    }
    return ret;
}

static QString randomQString(int length)
{
    static QString ch = QString::fromUtf8("あいうえおかきくけこさしすせよ亜有胃意宇江絵柄御尾化家機個去唆子詞鬆疎田痴釣就手貞途徒穫無那似煮塗縫之ノ");
    const int num = ch.length() - 1;
    QString ret;
    ret.reserve(length);

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(0, num)];
    }
    return ret;
}


class TestCmp : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void benchMemcmp1();
    void benchMemcmp2();
    void benchStrcmp1();
    void benchStrcmp2();
    void benchComparison1();
    void benchComparison2();
    void benchTfcmp1();
    void benchTfcmp2();
    void benchQStringCmp1();
    void benchQStringCmp2();
};

void TestCmp::initTestCase()
{
    str1 = randomString(LEN);
    str2 = randomString(LEN);
    str3 = str1;
    str3[LEN/2] = '_';

    qstr1 = randomQString(LEN);
    qstr2 = randomQString(LEN);
    qstr3 = qstr1;
    qstr3[LEN/2] = '_';
}

void TestCmp::benchMemcmp1()
{
    bool b = false;
    QBENCHMARK {
        b |= str1.length() == str2.length() && std::memcmp(str1.data(), str2.data(), LEN);
        b |= str2.length() == str1.length() && std::memcmp(str2.data(), str1.data(), LEN);
    }
    Q_UNUSED(b);
}

void TestCmp::benchMemcmp2()
{
    bool b = false;
    QBENCHMARK {
        b |= str1.length() == str3.length() && std::memcmp(str1.data(), str3.data(), LEN);
        b |= str3.length() == str1.length() && std::memcmp(str3.data(), str1.data(), LEN);
    }
    Q_UNUSED(b);
}

void TestCmp::benchStrcmp1()
{
    bool b = false;
    QBENCHMARK {
        b |= str1.length() == str2.length() && std::strncmp(str1.data(), str2.data(), LEN);
        b |= str2.length() == str1.length() && std::strncmp(str2.data(), str1.data(), LEN);
    }
    Q_UNUSED(b);
}

void TestCmp::benchStrcmp2()
{
    bool b = false;
    QBENCHMARK {
        b |= str1.length() == str3.length() && std::strncmp(str1.data(), str3.data(), LEN);
        b |= str3.length() == str1.length() && std::strncmp(str3.data(), str1.data(), LEN);
    }
    Q_UNUSED(b);
}

void TestCmp::benchComparison1()
{
    bool b = false;
    QBENCHMARK {
        b |= (str1 == str2);
        b |= (str2 == str1);
    }
    Q_UNUSED(b);
}

void TestCmp::benchComparison2()
{
    bool b = false;
    QBENCHMARK {
        b |= (str1 == str3);
        b |= (str3 == str1);
    }
    Q_UNUSED(b);
}

void TestCmp::benchTfcmp1()
{
    bool b = false;
    QBENCHMARK {
        b |= Tf::strcmp(str1, str2);
        b |= Tf::strcmp(str2, str1);
    }
    Q_UNUSED(b);
}

void TestCmp::benchTfcmp2()
{
    bool b = false;
    QBENCHMARK {
        b |= Tf::strcmp(str1, str3);
        b |= Tf::strcmp(str3, str1);
    }
    Q_UNUSED(b);
}

void TestCmp::benchQStringCmp1()
{
    bool b = false;
    QBENCHMARK {
        b |= (qstr1 == qstr2);
        b |= (qstr2 == qstr1);
    }
    Q_UNUSED(b);
}

void TestCmp::benchQStringCmp2()
{
    bool b = false;
    QBENCHMARK {
        b |= (qstr1 == qstr3);
        b |= (qstr3 == qstr1);
    }
    Q_UNUSED(b);
}


TF_TEST_MAIN(TestCmp)
#include "cmp.moc"
