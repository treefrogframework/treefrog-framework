/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmailmessage.h"
#include <QDateTime>
#include <QRegularExpression>
#include <THttpUtility>
#if QT_VERSION < 0x060000
# include <QTextCodec>
#endif
using namespace Tf;

constexpr auto DEFAULT_CONTENT_TYPE = "text/plain";

/*!
  \class TMailMessage
  \brief The TMailMessage class represents one email message.
*/

TMailMessage::TMailMessage(const TMailMessage &other) :
    TInternetMessageHeader(*static_cast<const TInternetMessageHeader *>(&other)),
    _mailBody(other._mailBody),
#if QT_VERSION < 0x060000
    _textCodec(other._textCodec),
#else
    _encoding(other._encoding),
#endif
    _recipientList(other._recipientList)
{
}


TMailMessage::TMailMessage(const QByteArray &encoding) :
    TInternetMessageHeader()
{
    init(encoding);
}


TMailMessage::TMailMessage(const char *encoding) :
    TInternetMessageHeader()
{
    init(encoding);
}


TMailMessage::TMailMessage(const QString &str, const QByteArray &encoding) :
    TInternetMessageHeader()
{
    init(encoding);
    parse(str);
}


void TMailMessage::init(const QByteArray &encoding)
{
#if QT_VERSION < 0x060000
    QTextCodec *codec = QTextCodec::codecForName(encoding);
    _textCodec = (codec) ? codec : QTextCodec::codecForName("UTF-8");
    // Sets default values
    setCurrentDate();
    QByteArray type = DEFAULT_CONTENT_TYPE;
    type += "; charset=\"";
    type += codec->name();
    type += '\"';
    setContentType(type);
#else
    QStringEncoder encoder(encoding.data());
    if (!encoder.isValid()) {
        encoder = QStringEncoder(QStringConverter::Utf8);
    }

    // Sets default values
    setCurrentDate();
    QByteArray type = DEFAULT_CONTENT_TYPE;
    type += "; charset=\"";
    type += encoder.name();
    type += '\"';
    setContentType(type);
#endif
}


inline int indexOfUsAscii(const QString &str, int from)
{
    for (int i = from; i < str.length(); ++i) {
        if (str[i].toLatin1() > 0)
            return i;
    }
    return -1;
}


void TMailMessage::parse(const QString &str)
{
    static const QRegularExpression rx("(\\n\\n|\\r\\n\\r\\n)");

    auto match = rx.match(str);
    int idx = match.capturedStart();
    int bdidx = idx + match.capturedLength();

    if (idx < 0) {
        tError("Not found mail headers");
        setBody(str);
    } else {
        QString header = str.left(idx);
        QByteArray ba;
        ba.reserve((int)(header.length() * 1.2));
        int i = 0;
        while (i < header.length()) {
            char c = header[i].toLatin1();
            if (c > 0) {
                ba += c;
                ++i;
            } else {  // not Latin-1 char
                int j = indexOfUsAscii(header, i);
                if (j < 0) {
                    j = header.length();
                }

#if QT_VERSION < 0x060000
                ba += THttpUtility::toMimeEncoded(header.mid(i, j - i), _textCodec);
#else
                ba += THttpUtility::toMimeEncoded(header.mid(i, j - i), _encoding);
#endif
                i = j;
            }
        }

        // Parses header
        TInternetMessageHeader::parse(ba);
        addRecipients(addresses("To"));
        addRecipients(addresses("Cc"));
        addRecipients(addresses("Bcc"));

        // Sets body
        QString body = str.mid(bdidx);
        setBody(body);
    }
}


QString TMailMessage::subject() const
{
    return THttpUtility::fromMimeEncoded(rawHeader("Subject"));
}


void TMailMessage::setSubject(const QString &subject)
{
#if QT_VERSION < 0x060000
    setRawHeader("Subject", THttpUtility::toMimeEncoded(subject, _textCodec));
#else
    setRawHeader("Subject", THttpUtility::toMimeEncoded(subject, _encoding));
#endif
}


void TMailMessage::addAddress(const QByteArray &field, const QByteArray &address, const QString &friendlyName)
{
    QByteArray addr = rawHeader(field);
    if (!addr.isEmpty()) {
        addr += ", ";
    }

    if (!friendlyName.isEmpty()) {
        QByteArray uname = friendlyName.toUtf8();
        if (uname.length() == friendlyName.length()) {
            addr += uname;
        } else {
            // multibyte char
#if QT_VERSION < 0x060000
            addr += THttpUtility::toMimeEncoded(friendlyName, _textCodec);
#else
            addr += THttpUtility::toMimeEncoded(friendlyName, _encoding);
#endif
        }
        addr += ' ';
    }

    addr += '<';
    addr += address.trimmed();
    addr += '>';
    setRawHeader(field, addr);
}


QByteArray TMailMessage::from() const
{
    return rawHeader("From");
}


QByteArray TMailMessage::fromAddress() const
{
    QByteArrayList addr = addresses("From");
    return addr.value(0);
}


QByteArrayList TMailMessage::addresses(const QByteArray &field) const
{
    QByteArrayList addrList;
    const QByteArrayList lst = rawHeader(field).split(',');

    for (const auto &ba : lst) {
        QByteArray addr;
        int i = ba.indexOf('<');
        if (i >= 0) {
            int j = ba.indexOf('>', ++i);
            if (j > i) {
                addr = ba.mid(i, j - i);
            }
        } else {
            addr = ba.trimmed();
        }

        if (!addr.isEmpty() && !addrList.contains(addr))
            addrList << addr;
    }
    return addrList;
}


void TMailMessage::setFrom(const QByteArray &address, const QString &friendlyName)
{
    removeAllRawHeaders("From");
    addAddress("From", address, friendlyName);
}


QByteArray TMailMessage::to() const
{
    return rawHeader("To");
}


void TMailMessage::addTo(const QByteArray &address, const QString &friendlyName)
{
    addAddress("To", address, friendlyName);
    addRecipient(address);
}


QByteArray TMailMessage::cc() const
{
    return rawHeader("Cc");
}


void TMailMessage::addCc(const QByteArray &address, const QString &friendlyName)
{
    addAddress("Cc", address, friendlyName);
    addRecipient(address);
}


QByteArray TMailMessage::bcc() const
{
    return rawHeader("Bcc");
}


void TMailMessage::addBcc(const QByteArray &address, const QString &friendlyName)
{
    addAddress("Bcc", address, friendlyName);
    addRecipient(address);
}


QString TMailMessage::body() const
{
    QByteArray ba = _mailBody;
    if (ba.contains(CRLF)) {
        ba.replace(CRLF, LF);
    }

#if QT_VERSION < 0x060000
    return _textCodec->toUnicode(ba);
#else
    return QStringDecoder(_encoding).decode(ba);
#endif
}


void TMailMessage::setBody(const QString &body)
{
#if QT_VERSION < 0x060000
    QByteArray ba = _textCodec->fromUnicode(body);
#else
    QByteArray ba = QStringEncoder(_encoding).encode(body);
#endif
    _mailBody.resize(0);
    _mailBody.reserve(ba.length() + ba.count('\n'));

    for (int i = 0; i < ba.length(); ++i) {
        if (ba[i] == '\n' && i > 0 && ba[i - 1] != '\r') {
            _mailBody += CRLF;
        } else {
            _mailBody += ba[i];
        }
    }
}


QByteArray TMailMessage::toByteArray() const
{
    return TInternetMessageHeader::toByteArray() + _mailBody;
}


void TMailMessage::addRecipient(const QByteArray &address)
{
    // Duplication check
    for (const auto &recp : (const QByteArrayList &)_recipientList) {
        if (recp == address)
            return;
    }
    _recipientList << address;
}


void TMailMessage::addRecipients(const QByteArrayList &addresses)
{
    for (const auto &addr : (const QByteArrayList &)addresses) {
        addRecipient(addr);
    }
}


TMailMessage &TMailMessage::operator=(const TMailMessage &other)
{
    TInternetMessageHeader::operator=(*static_cast<const TInternetMessageHeader *>(&other));
    _mailBody = other._mailBody;
#if QT_VERSION < 0x060000
    _textCodec = other._textCodec;  // codec static object
#else
    _encoding = other._encoding;
#endif
    _recipientList = other._recipientList;
    return *this;
}
