#include <TfTest/TfTest>
#include <QFile>
#include <TMultipartFormData>


class MultipartFormData : public QObject
{
    Q_OBJECT
private slots:
    void parse_data();
    void parse();
};


void MultipartFormData::parse_data()
{
     QTest::addColumn<QByteArray>("data");
     QTest::addColumn<QByteArray>("boundary");
     QTest::addColumn<QString>("name");
     QTest::addColumn<QString>("value");

     QTest::newRow("1") << QByteArray("-----------------------------168072824752491622650073\r\nContent-Disposition: form-data; name=\"authenticity_token\"\r\n\r\n446c9a7473ce606c75f0cd79cf16bbe1c0e185d8\r\n-----------------------------168072824752491622650073\r\nContent-Disposition: form-data; name=\"FiletoUpload\"; filename=\"kiban220430-1.pdf\"\r\nContent-Type: application/pdf\r\n\r\n%PDF-1.5^M%<E2><E3><CF><D3>870obj^M<</Linearized 1/L 149697/O 89/E 143703/N 1/T 149389/H [ 502 183]>>^Mendobj^M106 0 obj^M<</DecodeParms<</Columns 5/Predictor 12>>/Filter/FlateDecode/ID[<B7DE5E63EBAC4B4D9BCE2B8948438AEC><47A7B4819CDFDB4AB44FE127147B457A>]/Index[87 30]/Info 86 0 R/Length 97/Prev 149390/Root 88 0 R/Size 117/Type/XRef/W[1 3 1]>>streamh<DE>bbd<9A>^K^YESC<C1><A4>^X$^]D2^?\n")
                        << QByteArray("-----------------------------168072824752491622650073")
                        << "authenticity_token"
                        << "446c9a7473ce606c75f0cd79cf16bbe1c0e185d8";
}


void MultipartFormData::parse()
{
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, boundary);
    QFETCH(QString, name);
    QFETCH(QString, value);

    TMultipartFormData  formData(data, boundary);
    QString result = formData.formItemValue(name);
    QCOMPARE(result, value);
    // QString origName = formData.originalFileName("FiletoUpload");
    // QCOMPARE(origName, QString("kiban220430-1.pdf"));
    // bool ok = formData.renameUploadedFile("FiletoUpload", QString("/var/tmp/") + origName, true);
    // QCOMPARE(ok, true);
}


TF_TEST_MAIN(MultipartFormData)
#include "multipartformdata.moc"
