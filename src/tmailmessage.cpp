/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTextCodec>
#include <QDateTime>
#include <THttpUtility>
#include "tmailmessage.h"

#define DEFAULT_CONTENT_TYPE  "text/plain"

/*!
  \class TMailMessage
  \brief The TMailMessage class represents one email message.
*/

TMailMessage::TMailMessage(const TMailMessage &other)
    : TInternetMessageHeader(*static_cast<const TInternetMessageHeader *>(&other)),
      mailBody(other.mailBody),
      textCodec(other.textCodec),
      recipientList(other.recipientList)
{ }


TMailMessage::TMailMessage(const QByteArray &encoding)
    : TInternetMessageHeader(), textCodec(0)
{
    init(encoding);
}


TMailMessage::TMailMessage(const char *encoding)
    : TInternetMessageHeader(), textCodec(0)
{
    init(encoding);
}


TMailMessage::TMailMessage(const QString &str, const QByteArray &encoding)
    : TInternetMessageHeader(), textCodec(0)
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
    QRegExp rx("(\\n\\n|\\r\\n\\r\\n)", Qt::CaseSensitive, QRegExp::RegExp2);
    int idx = rx.indexIn(str, 0);
    int bdidx = idx + rx.matchedLength();

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
    QList<QByteArray> addr = addresses("From");
    return addr.value(0);
}


QList<QByteArray> TMailMessage::addresses(const QByteArray &field) const
{
    QList<QByteArray> addrList;
    const QList<QByteArray> lst = rawHeader(field).split(',');

    for (const auto &ba : lst) {
        QByteArray addr;
        int i = ba.indexOf('<');
        if (i >= 0) {
            int j = ba.indexOf('>', ++i);
            if (j > i) {
                addr =  ba.mid(i, j - i);
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
    if (ba.contains("\r\n"))
        ba.replace("\r\n", "\n");

    return textCodec->toUnicode(ba);
}


void TMailMessage::setBody(const QString &body)
{
    QByteArray ba = textCodec->fromUnicode(body);
    mailBody.clear();
    mailBody.reserve(ba.length() + ba.count('\n'));

    for (int i = 0; i < ba.length(); ++i) {
        if (ba[i] == '\n' && i > 0 && ba[i - 1] != '\r') {
            mailBody += "\r\n";
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
    for (const auto &recp : (const QList<QByteArray>&)recipientList) {
        if (recp == address)
            return;
    }
    recipientList << address;
}


void TMailMessage::addRecipients(const QList<QByteArray> &addresses)
{
    for (const auto &addr : (const QList<QByteArray>&)addresses) {
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
