/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsmtpmailer.h"
#include "tsystemglobal.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QSslSocket>
#include <TCryptMac>
#include <TPopMailer>
using namespace Tf;

//#define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)

/*!
  \class TSmtpMailer
  \brief The TSmtpMailer class provides a simple functionality to send
  emails by SMTP.
*/

TSmtpMailer::TSmtpMailer(QObject *parent) :
    QObject(parent),
    _socket(new QSslSocket())
{
}


TSmtpMailer::TSmtpMailer(const QString &hostName, quint16 port, QObject *parent) :
    QObject(parent),
    _socket(new QSslSocket()),
    _smtpHostName(hostName),
    _smtpPort(port)
{
}


TSmtpMailer::~TSmtpMailer()
{
    if (!_mailMessage.isEmpty()) {
        // tSystemWarn("Mail not sent. Deleted it.");
    }

    delete _pop;
    delete _socket;
}


void TSmtpMailer::moveToThread(QThread *targetThread)
{
    QObject::moveToThread(targetThread);
    _socket->moveToThread(targetThread);
    if (_pop) {
        _pop->moveToThread(targetThread);
    }
}


void TSmtpMailer::setPopBeforeSmtpAuthEnabled(const QString &popServer, quint16 port, bool apop, bool enable)
{
    if (enable) {
        if (!_pop) {
            _pop = new TPopMailer();
        }

        _pop->setHostName(popServer);
        _pop->setPort(port);
        _pop->setApopEnabled(apop);

    } else {
        delete _pop;
        _pop = nullptr;
    }
}


QString TSmtpMailer::lastServerResponse() const
{
    return QString(_lastResponse);
}


bool TSmtpMailer::send(const TMailMessage &message)
{
    _mailMessage = message;
    bool res = send();
    _mailMessage.clear();
    return res;
}


void TSmtpMailer::sendLater(const TMailMessage &message)
{
    _mailMessage = message;
    QMetaObject::invokeMethod(this, "sendAndDeleteLater", Qt::QueuedConnection);
}


void TSmtpMailer::sendAndDeleteLater()
{
    send();
    _mailMessage.clear();
    deleteLater();
}


bool TSmtpMailer::send()
{
    QMutexLocker locker(&_sendMutex);  // Lock for load reduction of mail server

    if (_pop) {
        // POP before SMTP
        _pop->setUserName(_username);
        _pop->setPassword(_password);
        _pop->connectToHost();
        _pop->quit();

        Tf::msleep(100);  // sleep
    }

    if (_smtpHostName.isEmpty() || _smtpPort <= 0) {
        tSystemError("SMTP: Bad Argument: hostname:%s port:%d", qUtf8Printable(_smtpHostName), _smtpPort);
        return false;
    }

    if (_mailMessage.fromAddress().trimmed().isEmpty()) {
        tSystemError("SMTP: Bad Argument: From-address empty");
        return false;
    }

    if (_mailMessage.recipients().isEmpty()) {
        tSystemError("SMTP: Bad Argument: Recipients empty");
        return false;
    }

    if (!connectToHost(_smtpHostName, _smtpPort)) {
        tSystemError("SMTP: Connect Error: hostname:%s port:%d", qUtf8Printable(_smtpHostName), _smtpPort);
        return false;
    }

    if (_mailMessage.date().isEmpty()) {
        _mailMessage.setCurrentDate();
    }

    if (!cmdEhlo()) {
        if (!cmdHelo()) {
            tSystemError("SMTP: HELO/EHLO Command Failed");
            cmdQuit();
            return false;
        }
    }

    if (_tlsRequire && !_tlsAvailable) {
        tSystemError("SMTP: STARTTLS not supported");
        cmdQuit();
        return false;
    }

    if (_tlsAvailable) {
        if (!cmdStartTls()) {
            cmdQuit();
            return false;
        }
    }

    if (_authEnable) {
        if (!cmdAuth()) {
            tSystemError("SMTP: User Authentication Failed: username:%s : [%s]", _username.data(), qUtf8Printable(lastServerResponse()));
            cmdQuit();
            return false;
        }
    }

    if (!cmdRset()) {
        tSystemError("SMTP: RSET Command Failed: [%s]", qUtf8Printable(lastServerResponse()));
        cmdQuit();
        return false;
    }

    if (!cmdMail(_mailMessage.fromAddress())) {
        tSystemError("SMTP: MAIL Command Failed: [%s]", qUtf8Printable(lastServerResponse()));
        cmdQuit();
        return false;
    }

    if (!cmdRcpt(_mailMessage.recipients())) {
        tSystemError("SMTP: RCPT Command Failed: [%s]", qUtf8Printable(lastServerResponse()));
        cmdQuit();
        return false;
    }

    if (!cmdData(_mailMessage.toByteArray())) {
        tSystemError("SMTP: DATA Command Failed: [%s]", qUtf8Printable(lastServerResponse()));
        cmdQuit();
        return false;
    }

    cmdQuit();
    return true;
}


QByteArray TSmtpMailer::authCramMd5(const QByteArray &in, const QByteArray &username, const QByteArray &password)
{
    QByteArray out = username;
    out += " ";
    out += TCryptMac::hash(QByteArray::fromBase64(in), password, TCryptMac::Hmac_Md5).toHex();
    return out.toBase64();
}


bool TSmtpMailer::connectToHost(const QString &hostName, quint16 port)
{
    _socket->connectToHost(hostName, port);
    if (!_socket->waitForConnected(5000)) {
        tSystemError("SMTP server connect error: %s", qUtf8Printable(_socket->errorString()));
        return false;
    }
    return (read() == 220);
}


bool TSmtpMailer::cmdEhlo()
{
    QByteArray ehlo;
    ehlo.append("EHLO [");
    ehlo.append(qUtf8Printable(_socket->localAddress().toString()));
    ehlo.append("]");

    QByteArrayList reply;
    if (cmd(ehlo, &reply) != 250) {
        return false;
    }

    // Gets AUTH methods
    for (auto &s : (const QByteArrayList &)reply) {
        QString str(s);
        if (str.startsWith("AUTH ", Qt::CaseInsensitive)) {
            _svrAuthMethods = str.mid(5).split(' ', Tf::SkipEmptyParts);
            tSystemDebug("AUTH: %s", qUtf8Printable(_svrAuthMethods.join(",")));
        }
        if (str.startsWith("STARTTLS", Qt::CaseInsensitive)) {
            _tlsAvailable = true;
        }
    }
    return true;
}


bool TSmtpMailer::cmdHelo()
{
    QByteArray helo;
    helo.append("HELO [");
    helo.append(qUtf8Printable(_socket->localAddress().toString()));
    helo.append("]");

    QByteArrayList reply;
    if (cmd(helo, &reply) != 250) {
        return false;
    }

    _tlsAvailable = false;
    _authEnable = false;
    return true;
}


bool TSmtpMailer::cmdStartTls()
{
    int code = cmd("STARTTLS");
    if (code != 220) {
        tSystemError("SMTP: STARTTLS failed [reply:%d]", code);
        return false;
    }

    _socket->startClientEncryption();
    if (!_socket->waitForEncrypted(5000)) {
        tSystemError("SMTP STARTTLS negotiation timeout: %s", qUtf8Printable(_socket->errorString()));
#if QT_VERSION >= 0x050f00  // 5.15.0
        auto errors = _socket->sslHandshakeErrors();
#else
        auto errors = _socket->sslErrors();
#endif
        for (const auto &err : errors) {
            tSystemError("SMTP SSL error : %s [%d]", qUtf8Printable(err.errorString()), (int)err.error());
        }
        return false;
    }

    if (!cmdEhlo()) {
        tSystemError("SMTP: EHLO Command Failed");
        cmdQuit();
        return false;
    }
    return true;
}


bool TSmtpMailer::cmdAuth()
{
    if (_svrAuthMethods.isEmpty())
        return true;

    if (_username.isEmpty() || _password.isEmpty()) {
        tSystemError("SMTP: AUTH Bad Argument: No username or password");
        return false;
    }

    QByteArrayList reply;
    QByteArray auth;
    bool res = false;

    // Try CRAM-MD5
    if (_svrAuthMethods.contains("CRAM-MD5", Qt::CaseInsensitive)) {
        auth = "AUTH CRAM-MD5";
        if (cmd(auth, &reply) == 334 && !reply.isEmpty()) {
            QByteArray md5 = authCramMd5(reply.first(), _username, _password);
            res = (cmd(md5) == 235);
        }
    }

    // Try LOGIN
    if (!res && _svrAuthMethods.contains("LOGIN", Qt::CaseInsensitive)) {
        auth = "AUTH LOGIN";
        if (cmd(auth) == 334 && cmd(_username.toBase64()) == 334 && cmd(_password.toBase64()) == 235) {
            res = true;
        }
    }

    // Try PLAIN
    if (!res && _svrAuthMethods.contains("PLAIN", Qt::CaseInsensitive)) {
        auth = "AUTH PLAIN ";
        auth += QByteArray().append(_username).append('\0').append(_username).append('\0').append(_password).toBase64();
        res = (cmd(auth) == 235);
    }

    return res;
}


bool TSmtpMailer::cmdRset()
{
    QByteArray rset("RSET");
    return (cmd(rset) == 250);
}


bool TSmtpMailer::cmdMail(const QByteArray &from)
{
    if (from.isEmpty())
        return false;

    QByteArray mail("MAIL FROM:<" + from + '>');
    return (cmd(mail) == 250);
}


bool TSmtpMailer::cmdRcpt(const QByteArrayList &to)
{
    if (to.isEmpty())
        return false;

    for (auto &tostr : to) {
        QByteArray rcpt("RCPT TO:<" + tostr + '>');
        if (cmd(rcpt) != 250) {
            return false;
        }
    }
    return true;
}


bool TSmtpMailer::cmdData(const QByteArray &message)
{
    QByteArray data("DATA");
    if (cmd(data) != 354) {
        return false;
    }
    return (cmd(message + CRLF + '.' + CRLF) == 250);
}


bool TSmtpMailer::cmdQuit()
{
    QByteArray quit("QUIT");
    return (cmd(quit) == 221);
}


int TSmtpMailer::cmd(const QByteArray &command, QByteArrayList *reply)
{
    _lastResponse.resize(0);
    if (!write(command))
        return -1;

    return read(reply);
}


bool TSmtpMailer::write(const QByteArray &command)
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


int TSmtpMailer::read(QByteArrayList *reply)
{
    if (reply) {
        reply->clear();
    }

    int code = 0;
    QByteArray rcv;
    for (;;) {
        rcv = _socket->readLine().trimmed();
        if (rcv.isEmpty()) {
            if (_socket->waitForReadyRead(5000)) {
                continue;
            } else {
                break;
            }
        }
        tSystemDebug("S: %s", rcv.data());

        if (code == 0)
            code = rcv.left(3).toInt();

        if (rcv.length() < 4)
            break;

        if (reply) {
            QByteArray ba = rcv.mid(4);
            if (!ba.isEmpty())
                *reply << ba;
        }

        if (code > 0 && rcv.at(3) == ' ')
            break;
    }
    _lastResponse = rcv;
    return code;
}


/*  Reply Codes

      211 System status, or system help reply
      214 Help message
          (Information on how to use the receiver or the meaning of a
          particular non-standard command; this reply is useful only
          to the human user)
      220 <domain> Service ready
      221 <domain> Service closing transmission channel
      235 Authentication successful
      250 Requested mail action okay, completed
      251 User not local; will forward to <forward-path>
      252 Cannot VRFY user, but will accept message and attempt
          delivery
      334 Continuation
      354 Start mail input; end with <CRLF>.<CRLF>
      421 <domain> Service not available, closing transmission channel
          (This may be a reply to any command if the service knows it
          must shut down)
      450 Requested mail action not taken: mailbox unavailable
          (e.g., mailbox busy)
      451 Requested action aborted: local error in processing
      452 Requested action not taken: insufficient system storage
      454 TLS not available due to temporary reason
      500 Syntax error, command unrecognized
          (This may include errors such as command line too long)
      501 Syntax error in parameters or arguments
      502 Command not implemented (see section 4.2.4)
      503 Bad sequence of commands
      504 Command parameter not implemented
      550 Requested action not taken: mailbox unavailable
          (e.g., mailbox not found, no access, or command rejected
          for policy reasons)
      551 User not local; please try <forward-path>
          (See section 3.4)
      552 Requested mail action aborted: exceeded storage allocation
      553 Requested action not taken: mailbox name not allowed
          (e.g., mailbox syntax incorrect)
      554 Transaction failed  (Or, in the case of a connection-opening
          response, "No SMTP service here")
*/
