/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TAccessLog>

/*!
  \class TAccessLog
  \brief The TAccessLog class defines the log of access to the web
  application server.
*/

TAccessLog::TAccessLog()
{
}


TAccessLog::TAccessLog(const QByteArray &host, const QByteArray &req) :
    remoteHost(host),
    request(req)
{
}


QByteArray TAccessLog::toByteArray(const QByteArray &layout, const QByteArray &dateTimeFormat) const
{
    QByteArray message;
    int pos = 0;
    QByteArray dig;
    message.reserve(127);

    while (pos < layout.length()) {
        char c = layout.at(pos++);
        if (c != '%') {
            message.append(c);
            continue;
        }

        dig.clear();
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

            switch (c) {
            case 'h':
                message.append(remoteHost);
                break;

            case 'd':  // %d : timestamp
                if (!dateTimeFormat.isEmpty()) {
                    message.append(timestamp.toString(dateTimeFormat).toLocal8Bit());
                } else {
                    message.append(timestamp.toString(Qt::ISODate).toLatin1());
                }
                break;

            case 'r':
                message.append(request);
                break;

            case 's':
                message.append(QString::number(statusCode).toLatin1());
                break;

            case 'O':
                if (dig.isEmpty()) {
                    message.append(QString::number(responseBytes).toLatin1());
                } else {
                    const QChar fillChar = (dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                    message.append(QString("%1").arg(responseBytes, dig.toInt(), 10, fillChar).toLatin1());
                }
                break;

            case 'n':  // %n : newline
                message.append('\n');
                break;

            case '%':
                message.append('%').append(dig);
                dig.clear();
                continue;
                break;

            default:
                message.append('%').append(dig).append(c);
                break;
            }
            break;
        }
    }
    return message;
}


TAccessLogger::TAccessLogger()
{
}


TAccessLogger::TAccessLogger(const TAccessLogger &other)
{
    if (other.accessLog) {
        open();
        *accessLog = *(other.accessLog);
    }
}


TAccessLogger::~TAccessLogger()
{
    close();
}


TAccessLogger &TAccessLogger::operator=(const TAccessLogger &other)
{
    if (other.accessLog) {
        open();
        *accessLog = *(other.accessLog);
    }
    return *this;
}


void TAccessLogger::open()
{
    if (!accessLog) {
        accessLog = new TAccessLog();
    }
}


void TAccessLogger::write()
{
    if (accessLog) {
        Tf::writeAccessLog(*accessLog);
    }
}


void TAccessLogger::close()
{
    delete accessLog;
    accessLog = nullptr;
}
