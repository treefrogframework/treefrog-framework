/* Copyright (c) 2014-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutex>
#include <TWebApplication>
#include "tsendmailmailer.h"
#include "tsystemglobal.h"

static QMutex sendMutex;

/*!
  \class TSendmailMailer
  \brief The TSendmailMailer class provides a simple functionality to send
  emails by sendmail command.
*/

TSendmailMailer::TSendmailMailer(const QString &command, QObject *parent)
    : QObject(parent), sendmailCmd(command), mailMessage()
{ }


TSendmailMailer::~TSendmailMailer()
{
    T_TRACEFUNC("");
    if (!mailMessage.isEmpty()) {
        tSystemWarn("Mail not sent. Deleted it.");
    }
}


bool TSendmailMailer::send(const TMailMessage &message)
{
    T_TRACEFUNC("");

    mailMessage = message;
    bool res = send();
    mailMessage.clear();
    return res;
}


void TSendmailMailer::sendLater(const TMailMessage &message)
{
    T_TRACEFUNC("");

    mailMessage = message;
    QMetaObject::invokeMethod(this, "sendAndDeleteLater", Qt::QueuedConnection);
}


void TSendmailMailer::sendAndDeleteLater()
{
    T_TRACEFUNC("");

    send();
    mailMessage.clear();
    deleteLater();
}


bool TSendmailMailer::send()
{
    QMutexLocker locker(&sendMutex); // Global lock for load reduction of mail server
    if (sendmailCmd.isEmpty()) {
        return false;
    }

    QStringList args;
    QByteArray rawmail = mailMessage.toByteArray();
    const QList<QByteArray> recipients = mailMessage.recipients();

    for (auto &recipt : recipients) {
        args.clear();
        args << recipt;

        QProcess sendmail;
        sendmail.start(sendmailCmd, args);
        if (!sendmail.waitForStarted(5000)) {
            tSystemError("Sendmail error. CMD: %s", qPrintable(sendmailCmd));
            return false;
        }

        sendmail.write(rawmail);
        sendmail.write("\n.\n");
        sendmail.waitForFinished();
        tSystemDebug("Mail sent. Recipients: %s", recipt.data());
    }

    return true;
}
