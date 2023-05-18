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
  \sa TActionMailer
*/

TPopMailer::TPopMailer(QObject *parent) :
    QObject(parent),
    _socket(new QTcpSocket)
{
}


TPopMailer::TPopMailer(const QString &hostName, uint16_t port, QObject *parent) :
    QObject(parent),
    _socket(new QTcpSocket),
    _popHostName(hostName),
    _popPort(port)
{
}


TPopMailer::~TPopMailer()
{
    delete _socket;
}


void TPopMailer::setHostName(const QString &hostName)
{
    _popHostName = hostName;
}


void TPopMailer::setPort(uint16_t port)
{
    _popPort = port;
}


void TPopMailer::setApopEnabled(bool enable)
{
    _apopEnabled = enable;
}


bool TPopMailer::connectToHost()
{
    bool ret = false;

    if (_popHostName.isEmpty() || _popPort <= 0) {
        tSystemError("POP: Bad Argument: hostname:%s port:%d", qUtf8Printable(_popHostName), _popPort);
        return ret;
    }

    _socket->connectToHost(_popHostName, _popPort);
    if (!_socket->waitForConnected(5000)) {
        tSystemError("POP server connect error: %s", qUtf8Printable(_socket->errorString()));
        return ret;
    }
    tSystemDebug("POP server connected: %s:%d", qUtf8Printable(_popHostName), _popPort);

    QByteArray response;
    readResponse(&response);

    QByteArray apopToken;
    int i = response.indexOf('<');
    int j = response.indexOf('>');
    if (i >= 0 && j > i) {
        apopToken = response.mid(i, j - i + 1);
        tSystemDebug("APOP token: %s", apopToken.data());
    }

    if (_apopEnabled) {
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
    user += _userName;
    return cmd(user);
}


bool TPopMailer::cmdPass()
{
    QByteArray pass("PASS ");
    pass += _password;
    return cmd(pass);
}


bool TPopMailer::cmdApop(const QByteArray &token)
{
    QByteArray apop("APOP ");
    apop += _userName;
    apop += ' ';
    QByteArray data = token + _password;
    apop += QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
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
        while (_socket->waitForReadyRead(5000)) {
            message += _socket->readAll();
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

    int len = _socket->write(cmd);
    _socket->flush();
    tSystemDebug("C: %s", cmd.trimmed().data());
    return (len == cmd.length());
}


bool TPopMailer::readResponse(QByteArray *reply)
{
    bool ret = false;

    if (reply) {
        reply->resize(0);
    }

    if (_socket->waitForReadyRead(5000)) {
        QByteArray rcv = _socket->readLine();
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
