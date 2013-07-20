/* Copyright (c) 2011-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAccessLog>

/*!
  \class TAccessLog
  \brief The TAccessLog class defines the log of access to the web
  application server.
*/

TAccessLog::TAccessLog()
    : timestamp(), remoteHost(), request(), statusCode(0), responseBytes(0)
{ }


TAccessLog::TAccessLog(const QByteArray &host, const QByteArray &req)
    : timestamp(QDateTime::currentDateTime()), remoteHost(host), request(req), statusCode(0), responseBytes(0)
{ }


QByteArray TAccessLog::toByteArray(const QByteArray &layout, const QByteArray &dateTimeFormat) const
{
    QByteArray message;
    int pos = 0;
    while (pos < layout.length()) {
        char c = layout.at(pos++);
        if (c != '%') {
            message.append(c);
            continue;
        }

        QByteArray dig;
        for (;;) {
            if (pos >= layout.length()) {
                message.append('%').append(dig);
                break;
            }

            c = layout.at(pos++);
            if (c >= '0' && c <= '9') {
                dig += c;
                continue;
            }

            if (c == 'h') {
                message.append(remoteHost);

            } else if (c == 'd') {  // %d : timestamp
                if (!dateTimeFormat.isEmpty()) {
                    message.append(timestamp.toString(dateTimeFormat).toLocal8Bit());
                } else {
                    message.append(timestamp.toString(Qt::ISODate).toLatin1());
                }

            } else if (c == 'r') {
                message.append(request);

            } else if (c == 's') {
                message.append(QString::number(statusCode));

            } else if (c == 'O') {
                message.append(QString::number(responseBytes));

            } else if (c == 'n') {  // %n : newline
                message.append('\n');

            } else if (c == '%') {
                message.append('%').append(dig);
                dig.clear();
                continue;
            } else {
                message.append('%').append(dig).append(c);
            }
            break;
        }
    }
    return message;
}
