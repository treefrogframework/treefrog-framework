/* Copyright (c) 2014-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsendmailmailer.h"
#include "tsystemglobal.h"
#include <QMutex>
#include <TWebApplication>

namespace {
QMutex sendMutex;
}

/*!
  \class TSendmailMailer
  \brief The TSendmailMailer class provides a simple functionality to send
  emails by sendmail command.
*/

TSendmailMailer::TSendmailMailer(const QString &command, QObject *parent) :
    QObject(parent),
    sendmailCmd(command),
    mailMessage()
{
}


TSendmailMailer::~TSendmailMailer()
{
    if (!mailMessage.isEmpty()) {
        tSystemWarn("Mail not sent. Deleted it.");
    }
}


bool TSendmailMailer::send(const TMailMessage &message)
{
    mailMessage = message;
    bool res = send();
    mailMessage.clear();
    return res;
}


void TSendmailMailer::sendLater(const TMailMessage &message)
{
    mailMessage = message;
    QMetaObject::invokeMethod(this, "sendAndDeleteLater", Qt::QueuedConnection);
}


void TSendmailMailer::sendAndDeleteLater()
{
    send();
    mailMessage.clear();
    deleteLater();
}


bool TSendmailMailer::send()
{
    QMutexLocker locker(&sendMutex);  // Global lock for load reduction of mail server
    if (sendmailCmd.isEmpty()) {
        return false;
    }

    QStringList args;
    QByteArray rawmail = mailMessage.toByteArray();
    const QByteArrayList recipients = mailMessage.recipients();

    for (auto &recipt : recipients) {
        args.clear();
        args << recipt;

        QProcess sendmail;
        sendmail.start(sendmailCmd, args);
        if (!sendmail.waitForStarted(5000)) {
            tSystemError("Sendmail error. CMD: %s", qUtf8Printable(sendmailCmd));
            return false;
        }

        sendmail.write(rawmail);
        sendmail.write("\n.\n");
        sendmail.waitForFinished();
        tSystemDebug("Mail sent. Recipients: %s", recipt.data());
    }

    return true;
}
