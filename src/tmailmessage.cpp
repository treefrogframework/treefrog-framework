/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmailmessage.h"
#include <QDateTime>
#include <QTextCodec>
#include <QRegularExpression>
#include <THttpUtility>
using namespace Tf;

constexpr auto DEFAULT_CONTENT_TYPE = "text/plain";

/*!
  \class TMailMessage
  \brief The TMailMessage class represents one email message.
*/

TMailMessage::TMailMessage(const TMailMessage &other) :
    TInternetMessageHeader(*static_cast<const TInternetMessageHeader *>(&other)),
    mailBody(other.mailBody),
    textCodec(other.textCodec),
    recipientList(other.recipientList)
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
    QTextCodec *codec = QTextCodec::codecForName(encoding);
    textCodec = (codec) ? codec : QTextCodec::codecForName("UTF-8");
    // Sets default values
    setCurrentDate();
    QByteArray type = DEFAULT_CONTENT_TYPE;
    type += "; charset=\"";
    type += codec->name();
    type += '\"';
    setContentType(type);
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

                ba += THttpUtility::toMimeEncoded(header.mid(i, j - i), textCodec);
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
    setRawHeader("Subject", THttpUtility::toMimeEncoded(subject, textCodec));
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
            addr += THttpUtility::toMimeEncoded(friendlyName, textCodec);
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
    QByteArray ba = mailBody;
    if (ba.contains(CRLF)) {
        ba.replace(CRLF, LF);
    }
    return textCodec->toUnicode(ba);
}


void TMailMessage::setBody(const QString &body)
{
    QByteArray ba = textCodec->fromUnicode(body);
    mailBody.resize(0);
    mailBody.reserve(ba.length() + ba.count('\n'));

    for (int i = 0; i < ba.length(); ++i) {
        if (ba[i] == '\n' && i > 0 && ba[i - 1] != '\r') {
            mailBody += CRLF;
        } else {
            mailBody += ba[i];
        }
    }
}


QByteArray TMailMessage::toByteArray() const
{
    return TInternetMessageHeader::toByteArray() + mailBody;
}


void TMailMessage::addRecipient(const QByteArray &address)
{
    // Duplication check
    for (const auto &recp : (const QByteArrayList &)recipientList) {
        if (recp == address)
            return;
    }
    recipientList << address;
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
    mailBody = other.mailBody;
    textCodec = other.textCodec;  // codec static object
    recipientList = other.recipientList;
    return *this;
}
