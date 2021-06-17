/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tpopmailer.h"
#include "tsystemglobal.h"
#include <QCryptographicHash>
#include <QTcpSocket>
using namespace Tf;

/*!
  \class TPopMailer
  \brief The TPopMailer class provides a simple functionality to receive
  emails by POP.
*/

TPopMailer::TPopMailer(QObject *parent) :
    QObject(parent),
    socket(new QTcpSocket)
{
}


TPopMailer::TPopMailer(const QString &hostName, quint16 port, QObject *parent) :
    QObject(parent),
    socket(new QTcpSocket),
    popHostName(hostName),
    popPort(port)
{
}


TPopMailer::~TPopMailer()
{
    delete socket;
}


void TPopMailer::setHostName(const QString &hostName)
{
    popHostName = hostName;
}


void TPopMailer::setPort(quint16 port)
{
    popPort = port;
}


void TPopMailer::setApopEnabled(bool enable)
{
    apopEnabled = enable;
}


bool TPopMailer::connectToHost()
{
    bool ret = false;

    if (popHostName.isEmpty() || popPort <= 0) {
        tSystemError("POP: Bad Argument: hostname:%s port:%d", qUtf8Printable(popHostName), popPort);
        return ret;
    }

    socket->connectToHost(popHostName, popPort);
    if (!socket->waitForConnected(5000)) {
        tSystemError("POP server connect error: %s", qUtf8Printable(socket->errorString()));
        return ret;
    }
    tSystemDebug("POP server connected: %s:%d", qUtf8Printable(popHostName), popPort);

    QByteArray response;
    readResponse(&response);

    QByteArray apopToken;
    int i = response.indexOf('<');
    int j = response.indexOf('>');
    if (i >= 0 && j > i) {
        apopToken = response.mid(i, j - i + 1);
        tSystemDebug("APOP token: %s", apopToken.data());
    }

    if (apopEnabled) {
        // APOP authentication
        ret = cmdApop(apopToken);
    } else {
        // POP authentication
        ret = (cmdUser() && cmdPass());
    }

    if (!ret) {
        tSystemWarn("POP authorization failed");
    }

    return ret;
}


QByteArray TPopMailer::readMail(int index)
{
    QByteArray msg;
    cmdRetr(index, msg);
    return msg;
}


void TPopMailer::quit()
{
    cmdQuit();
}


bool TPopMailer::cmdUser()
{
    QByteArray user("USER ");
    user += userName;
    return cmd(user);
}


bool TPopMailer::cmdPass()
{
    QByteArray pass("PASS ");
    pass += password;
    return cmd(pass);
}


bool TPopMailer::cmdApop(const QByteArray &token)
{
    QByteArray apop("APOP ");
    apop += userName;
    apop += ' ';
    apop += QCryptographicHash::hash(token + password, QCryptographicHash::Md5).toHex();
    return cmd(apop);
}


bool TPopMailer::cmdStat()
{
    QByteArray stat("STAT");
    return cmd(stat);
}


bool TPopMailer::cmdRetr(int index, QByteArray &message)
{
    QByteArray retr("RETR ");
    retr += QByteArray::number(index);
    message.resize(0);
    const QByteArray eol = QByteArrayLiteral(".") + CRLF;

    bool res = cmd(retr);
    if (res) {
        while (socket->waitForReadyRead(5000)) {
            message += socket->readAll();
            if (message.endsWith(eol)) {
                break;
            }
        }
    }
    return res;
}


bool TPopMailer::cmdQuit()
{
    QByteArray quit("QUIT");
    return cmd(quit);
}


bool TPopMailer::cmd(const QByteArray &command, QByteArray *reply)
{
    if (!write(command))
        return false;

    return readResponse(reply);
}


bool TPopMailer::write(const QByteArray &command)
{
    QByteArray cmd = command;
    if (!cmd.endsWith(CRLF)) {
        cmd += CRLF;
    }

    int len = socket->write(cmd);
    socket->flush();
    tSystemDebug("C: %s", cmd.trimmed().data());
    return (len == cmd.length());
}


bool TPopMailer::readResponse(QByteArray *reply)
{
    bool ret = false;

    if (reply) {
        reply->resize(0);
    }

    if (socket->waitForReadyRead(5000)) {
        QByteArray rcv = socket->readLine();
        tSystemDebug("S: %s", rcv.data());

        if (rcv.startsWith("+OK")) {
            ret = true;
            if (reply) {
                *reply = rcv.mid(3).trimmed();
            }
        } else if (rcv.startsWith("-ERR")) {
            if (reply) {
                *reply = rcv.mid(4).trimmed();
            }
        } else {
            tSystemError("S: %s", rcv.data());
        }
    }
    return ret;
}
