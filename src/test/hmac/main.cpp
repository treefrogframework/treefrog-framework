#include <QTest>
#include <QByteArray>
#include <tsmtpmailer.h>
#include <tcryptmac.h>


class TestHMAC : public QObject
{
    Q_OBJECT
private slots:
    void hmacmd5_data();
    void hmacmd5();
    void hmacsha1_data();
    void hmacsha1();
#if QT_VERSION >= 0x050000
    void hmacsha256_data();
    void hmacsha256();
    void hmacsha384_data();
    void hmacsha384();
    void hmacsha512_data();
    void hmacsha512();
#endif
    void crammd5();
};


void TestHMAC::hmacmd5_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                       << QByteArray("Hi There")
                       << QByteArray::fromHex("9294727a3638bb1c13f48ef8158bfc9d");

    QTest::newRow("2") << QByteArray("Jefe")
                       << QByteArray("what do ya want for nothing?")
                       << QByteArray::fromHex("750c783e6ab0b503eaa86e310a5db738");

    QTest::newRow("3") << QByteArray(16, 0xaa)
                       << QByteArray(50, 0xdd)
                       << QByteArray::fromHex("56be34521d144c88dbb8c733f0e8b3f6");

    QTest::newRow("4") << QByteArray::fromHex("0102030405060708090a0b0c0d0e0f10111213141516171819")
                       << QByteArray(50, 0xcd)
                       << QByteArray::fromHex("697eaf0aca3a3aea3a75164746ffaa79");

    QTest::newRow("5") << QByteArray::fromHex("0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c")
                       << QByteArray("Test With Truncation")
                       << QByteArray::fromHex("56461ef2342edc00f9bab995690efd4c");

    QTest::newRow("6") << QByteArray(80, 0xaa)
                       << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
                       << QByteArray::fromHex("6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd");

    QTest::newRow("7") << QByteArray(80, 0xaa)
                       << QByteArray("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data")
                       << QByteArray::fromHex("6f630fad67cda0ee1fb1f562db3aa53e");
}


void TestHMAC::hmacmd5()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, text);
    QFETCH(QByteArray, result);

    QByteArray actual = TCryptMac::hash(text, key, TCryptMac::Hmac_Md5);
    QCOMPARE(actual, result);
}


void TestHMAC::hmacsha1_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                       << QByteArray("Hi There")
                       << QByteArray::fromHex("b617318655057264e28bc0b6fb378c8ef146be00");

    QTest::newRow("2") << QByteArray("Jefe")
                       << QByteArray("what do ya want for nothing?")
                       << QByteArray::fromHex("effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");

    QTest::newRow("3") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray(50, 0xdd)
                       << QByteArray::fromHex("125d7342b9ac11cd91a39af48aa17b4f63f175d3");

    QTest::newRow("4") << QByteArray::fromHex("0102030405060708090a0b0c0d0e0f10111213141516171819")
                       << QByteArray(50, 0xcd)
                       << QByteArray::fromHex("4c9007f4026250c6bc8414f9bf50c86c2d7235da");

    QTest::newRow("5") << QByteArray::fromHex("0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c")
                       << QByteArray("Test With Truncation")
                       << QByteArray::fromHex("4c1a03424b55e07fe7f27be1d58bb9324a9a5a04");

    QTest::newRow("6") << QByteArray(80, 0xaa)
                       << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
                       << QByteArray::fromHex("aa4ae5e15272d00e95705637ce8a3b55ed402112");

    QTest::newRow("7") << QByteArray(80, 0xaa)
                       << QByteArray("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data")
                       << QByteArray::fromHex("e8e99d0f45237d786d6bbaa7965c7808bbff1a91");
}


void TestHMAC::hmacsha1()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, text);
    QFETCH(QByteArray, result);

    QByteArray actual = TCryptMac::hash(text, key, TCryptMac::Hmac_Sha1);
    QCOMPARE(actual, result);
}

#if QT_VERSION >= 0x050000

void TestHMAC::hmacsha256_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray("key")
                       << QByteArray("The quick brown fox jumps over the lazy dog")
                       << QByteArray::fromHex("f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
    QTest::newRow("2") << QByteArray("Jefe")
                       << QByteArray("what do ya want for nothing?")
                       << QByteArray::fromHex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    QTest::newRow("3") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
                       << QByteArray::fromHex("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    QTest::newRow("4") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.")
                       << QByteArray::fromHex("9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");
}


void TestHMAC::hmacsha256()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, text);
    QFETCH(QByteArray, result);

    QByteArray actual = TCryptMac::hash(text, key, TCryptMac::Hmac_Sha256);
    QCOMPARE(actual, result);
}


void TestHMAC::hmacsha384_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray("Jefe")
                       << QByteArray("what do ya want for nothing?")
                       << QByteArray::fromHex("af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e8e2240ca5e69e2c78b3239ecfab21649");
    QTest::newRow("2") << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                       << QByteArray("Hi There")
                       << QByteArray::fromHex("afd03944d84895626b0825f4ab46907f15f9dadbe4101ec682aa034c7cebc59cfaea9ea9076ede7f4af152e8b2fa9cb6");
    QTest::newRow("3") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
                       << QByteArray::fromHex("4ece084485813e9088d2c63a041bc5b44f9ef1012a2b588f3cd11f05033ac4c60c2ef6ab4030fe8296248df163f44952");
    QTest::newRow("4") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.")
                       << QByteArray::fromHex("6617178e941f020d351e2f254e8fd32c602420feb0b8fb9adccebb82461e99c5a678cc31e799176d3860e6110c46523e");
}


void TestHMAC::hmacsha384()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, text);
    QFETCH(QByteArray, result);

    QByteArray actual = TCryptMac::hash(text, key, TCryptMac::Hmac_Sha384);
    QCOMPARE(actual, result);
}


void TestHMAC::hmacsha512_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("1") << QByteArray("Jefe")
                       << QByteArray("what do ya want for nothing?")
                       << QByteArray::fromHex("164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737");
    QTest::newRow("2") << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                       << QByteArray("Hi There")
                       << QByteArray::fromHex("87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854");
    QTest::newRow("3") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("Test Using Larger Than Block-Size Key - Hash Key First")
                       << QByteArray::fromHex("80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598");
    QTest::newRow("4") << QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
                       << QByteArray("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.")
                       << QByteArray::fromHex("e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58");
}


void TestHMAC::hmacsha512()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, text);
    QFETCH(QByteArray, result);

    QByteArray actual = TCryptMac::hash(text, key, TCryptMac::Hmac_Sha512);
    QCOMPARE(actual, result);
}

#endif

void TestHMAC::crammd5()
{
    QByteArray in("PDI3MTIwMDQ1MjcuOTEyMDI2MUBzbXRwMTAuZHRpLm5lLmpwPg==");
    QByteArray result("a2F6em5Ab3BzLmR0aS5uZS5qcCA4YTQwY2FmZmVlODRkZjMwZWI2ZWMxMjMyNmYzYWRiOA==");
    QByteArray actual = TSmtpMailer::authCramMd5(in, "kazzn@ops.dti.ne.jp", "kazu27");
    QCOMPARE(actual, result);
}


QTEST_APPLESS_MAIN(TestHMAC)
#include "main.moc"


/*
2. Test Cases for HMAC-MD5

test_case =     1
key =           0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
key_len =       16
data =          "Hi There"
data_len =      8
digest =        0x9294727a3638bb1c13f48ef8158bfc9d

test_case =     2
key =           "Jefe"
key_len =       4
data =          "what do ya want for nothing?"
data_len =      28
digest =        0x750c783e6ab0b503eaa86e310a5db738

test_case =     3
key =           0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
key_len         16
data =          0xdd repeated 50 times
data_len =      50
digest =        0x56be34521d144c88dbb8c733f0e8b3f6

test_case =     4
key =           0x0102030405060708090a0b0c0d0e0f10111213141516171819
key_len         25
data =          0xcd repeated 50 times
data_len =      50
digest =        0x697eaf0aca3a3aea3a75164746ffaa79

test_case =     5
key =           0x0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c
key_len =       16
data =          "Test With Truncation"
data_len =      20
digest =        0x56461ef2342edc00f9bab995690efd4c
digest-96       0x56461ef2342edc00f9bab995

test_case =     6
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key - Hash Key First"
data_len =      54
digest =        0x6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd

test_case =     7
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key and Larger
                Than One Block-Size Data"
data_len =      73
digest =        0x6f630fad67cda0ee1fb1f562db3aa53e



3. Test Cases for HMAC-SHA-1

test_case =     1
key =           0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
key_len =       20
data =          "Hi There"
data_len =      8
digest =        0xb617318655057264e28bc0b6fb378c8ef146be00

test_case =     2
key =           "Jefe"
key_len =       4
data =          "what do ya want for nothing?"
data_len =      28
digest =        0xeffcdf6ae5eb2fa2d27416d5f184df9c259a7c79

test_case =     3
key =           0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
key_len =       20
data =          0xdd repeated 50 times
data_len =      50
digest =        0x125d7342b9ac11cd91a39af48aa17b4f63f175d3

test_case =     4
key =           0x0102030405060708090a0b0c0d0e0f10111213141516171819
key_len =       25
data =          0xcd repeated 50 times
data_len =      50
digest =        0x4c9007f4026250c6bc8414f9bf50c86c2d7235da

test_case =     5
key =           0x0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c
key_len =       20
data =          "Test With Truncation"
data_len =      20
digest =        0x4c1a03424b55e07fe7f27be1d58bb9324a9a5a04
digest-96 =     0x4c1a03424b55e07fe7f27be1

test_case =     6
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key - Hash Key First"
data_len =      54
digest =        0xaa4ae5e15272d00e95705637ce8a3b55ed402112

test_case =     7
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key and Larger
                Than One Block-Size Data"
data_len =      73
digest =        0xe8e99d0f45237d786d6bbaa7965c7808bbff1a91
data_len =      20
digest =        0x4c1a03424b55e07fe7f27be1d58bb9324a9a5a04
digest-96 =     0x4c1a03424b55e07fe7f27be1

test_case =     6
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key - Hash Key
First"
data_len =      54
digest =        0xaa4ae5e15272d00e95705637ce8a3b55ed402112

test_case =     7
key =           0xaa repeated 80 times
key_len =       80
data =          "Test Using Larger Than Block-Size Key and Larger
                Than One Block-Size Data"
data_len =      73
digest =        0xe8e99d0f45237d786d6bbaa7965c7808bbff1a91
*/


/*
CRAM-MD5 ex.
 S: 334 PDI3MTIwMDQ1MjcuOTEyMDI2MUBzbXRwMTAuZHRpLm5lLmpwPg==
 C: a2F6em5Ab3BzLmR0aS5uZS5qcCA4YTQwY2FmZmVlODRkZjMwZWI2ZWMxMjMyNmYzYWRiOA==


#!/bin/sh
CH=`echo -n 'PDI3MTIwMDQ1MjcuOTEyMDI2MUBzbXRwMTAuZHRpLm5lLmpwPg==' | base64 -d`
MD5=`echo -n "$CH" | openssl md5 -hmac (password)`
echo -n "account@example.co.jp $MD5" | base64

 */
