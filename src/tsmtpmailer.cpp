/* Copyright (c) 2011-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTcpSocket>
#include <QHostAddress>
#include <QStringListIterator>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <QCoreApplication>
#include <TCryptMac>
#include <TPopMailer>
#include "tsmtpmailer.h"
#include "tsystemglobal.h"

#if defined(Q_OS_WIN)
#  define CRLF "\n"
#else
#  define CRLF "\r\n"
#endif

static QMutex sendMutex;

/*!
  \class TSmtpMailer
  \brief The TSmtpMailer class provides a simple functionality to send
  emails by SMTP.
*/

TSmtpMailer::TSmtpMailer(QObject *parent)
    : QObject(parent), socket(new QTcpSocket), smtpPort(0), authEnable(false), pop(0)
{ }


TSmtpMailer::TSmtpMailer(const QString &hostName, quint16 port, QObject *parent)
    : QObject(parent), socket(new QTcpSocket), smtpHostName(hostName), smtpPort(port),
      authEnable(false), pop(0)
{ }


TSmtpMailer::~TSmtpMailer()
{
    T_TRACEFUNC("");
    if (!mailMessage.isEmpty()) {
//        tSystemWarn("Mail not sent. Deleted it.");
    }

    if (pop)
        delete pop;

    delete socket;
}


void TSmtpMailer::setHostName(const QString &hostName)
{
    smtpHostName = hostName;
}


void TSmtpMailer::setPort(quint16 port)
{
    smtpPort = port;
}


void TSmtpMailer::setPopBeforeSmtpAuthEnabled(const QString &popServer, quint16 port, bool apop, bool enable)
{
    if (enable) {
        if (!pop) {
            pop = new TPopMailer();
        }

        pop->setHostName(popServer);
        pop->setPort(port);
        pop->setApopEnabled(apop);

    } else {
        if (pop) {
            delete pop;
        }
        pop = NULL;
    }
}


bool TSmtpMailer::send(const TMailMessage &message)
{
    T_TRACEFUNC("");

    mailMessage = message;
    bool res = send();
    mailMessage.clear();
    return res;
}


void TSmtpMailer::sendLater(const TMailMessage &message)
{
    T_TRACEFUNC("");

    mailMessage = message;
    QTimer::singleShot(0, this, SLOT(sendAndDeleteLater()));
}


void TSmtpMailer::sendAndDeleteLater()
{
    T_TRACEFUNC("");

    send();
    mailMessage.clear();
    deleteLater();
}


bool TSmtpMailer::send()
{
    QMutexLocker locker(&sendMutex); // Global lock for load reduction of mail server

    if (pop) {
        // POP before SMTP
        pop->setUserName(userName);
        pop->setPassword(password);
        pop->connectToHost();
        pop->quit();

        Tf::msleep(100); // sleep
    }

    if (smtpHostName.isEmpty() || smtpPort <= 0) {
        tSystemError("SMTP: Bad Argument: hostname:%s port:%d", qPrintable(smtpHostName), smtpPort);
        return false;
    }

    if (mailMessage.fromAddress().trimmed().isEmpty()) {
        tSystemError("SMTP: Bad Argument: From-address empty");
        return false;
    }

    if (mailMessage.recipients().isEmpty()) {
        tSystemError("SMTP: Bad Argument: Recipients empty");
        return false;
    }

    if (!connectToHost(smtpHostName, smtpPort)) {
        tSystemError("SMTP: Connect Error: hostname:%s port:%d", qPrintable(smtpHostName), smtpPort);
        return false;
    }

    if (mailMessage.date().isEmpty()) {
        mailMessage.setCurrentDate();
    }

    if (!cmdEhlo()) {
        tSystemError("SMTP: EHLO Command Failed");
        cmdQuit();
        return false;
    }

    if (authEnable) {
        if (!cmdAuth()) {
            tSystemError("SMTP: User Authentication Failed: username:%s", userName.data());
            cmdQuit();
            return false;
        }
    }

    if (!cmdRset()) {
        tSystemError("SMTP: RSET Command Failed");
        cmdQuit();
        return false;
    }

    if (!cmdMail(mailMessage.fromAddress())) {
        tSystemError("SMTP: MAIL Command Failed");
        cmdQuit();
        return false;
    }

    if (!cmdRcpt(mailMessage.recipients())) {
        tSystemError("SMTP: RCPT Command Failed");
        cmdQuit();
        return false;
    }

    if (!cmdData(mailMessage.toByteArray())) {
        tSystemError("SMTP: DATA Command Failed");
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
    out += TCryptMac::mac(QByteArray::fromBase64(in), password, TCryptMac::Hmac_Md5).toHex();
    return out.toBase64();
}


bool TSmtpMailer::connectToHost(const QString &hostName, quint16 port)
{
    socket->connectToHost(hostName, port);
    if (!socket->waitForConnected(5000)) {
        tSystemError("SMTP server connect error: %s", qPrintable(socket->errorString()));
        return false;
    }
    return (read() == 220);
}


bool TSmtpMailer::cmdEhlo()
{
    QByteArray ehlo;
    ehlo.append("EHLO [");
    ehlo.append(qPrintable(socket->localAddress().toString()));
    ehlo.append("]");

    QList<QByteArray> reply;
    if (cmd(ehlo, &reply) != 250) {
        return false;
    }

    // Gets AUTH methods
    for (QListIterator<QByteArray> i(reply); i.hasNext(); ) {
        QString str(i.next());
        if (str.startsWith("AUTH ", Qt::CaseInsensitive)) {
            svrAuthMethods = str.mid(5).split(' ', QString::SkipEmptyParts);
            tSystemDebug("AUTH: %s", qPrintable(svrAuthMethods.join(",")));
            break;
        }
    }
    return true;
}


bool TSmtpMailer::cmdAuth()
{
    if (svrAuthMethods.isEmpty())
        return true;

    if (userName.isEmpty() || password.isEmpty()) {
        tSystemError("SMTP: AUTH Bad Argument: No username or password");
        return false;
    }

    QList<QByteArray> reply;
    QByteArray auth;
    bool res = false;

    // Try CRAM-MD5
    if (svrAuthMethods.contains("CRAM-MD5", Qt::CaseInsensitive)) {
        auth = "AUTH CRAM-MD5";
        if (cmd(auth, &reply) == 334 && !reply.isEmpty()) {
            QByteArray md5 = authCramMd5(reply.first(), userName, password);
            res = (cmd(md5) == 235);
        }
    }

    // Try LOGIN
    if (!res && svrAuthMethods.contains("LOGIN", Qt::CaseInsensitive)) {
        auth = "AUTH LOGIN";
        if (cmd(auth) == 334 && cmd(userName.toBase64()) == 334 && cmd(password.toBase64()) == 235) {
            res = true;
        }
    }

    // Try PLAIN
    if (!res && svrAuthMethods.contains("PLAIN", Qt::CaseInsensitive)) {
        auth = "AUTH PLAIN ";
        auth += QByteArray().append(userName).append('\0').append(userName).append('\0').append(password).toBase64();
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


bool TSmtpMailer::cmdRcpt(const QList<QByteArray> &to)
{
    if (to.isEmpty())
        return false;

    for (QListIterator<QByteArray> i(to); i.hasNext(); ) {
        QByteArray rcpt("RCPT TO:<" + i.next() + '>');
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


int TSmtpMailer::cmd(const QByteArray &command, QList<QByteArray> *reply)
{
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

    int len = socket->write(cmd);
    socket->flush();
    tSystemDebug("C: %s", cmd.trimmed().data());
    return (len == cmd.length());
}


int TSmtpMailer::read(QList<QByteArray> *reply)
{
    if (reply)
        reply->clear();

    int code = 0;
    for (;;) {
        QByteArray rcv = socket->readLine().trimmed();
        if (rcv.isEmpty()) {
            if (socket->waitForReadyRead(5000)) {
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




