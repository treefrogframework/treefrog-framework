/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QLocalSocket>
#include <QTimerEvent>
#include <TActionProcess>
#include "tlogsender.h"

const int QUEUE_MAX_SIZE = 1024 * 1024; // bytes


TLogSender::TLogSender(const QString &server)
    : QObject(), serverName(server), socket(new QLocalSocket(this))
{
    reopen();
}


TLogSender::~TLogSender()
{
    socket->flush();
    socket->close();
}


void TLogSender::reopen()
{
    if (socket->state() != QLocalSocket::UnconnectedState) {
        socket->abort();
    }

    socket->connectToServer(serverName, QIODevice::WriteOnly);
}


void TLogSender::writeLog(const QByteArray &log)
{
    static bool forkFlag = false;

    // fork check
    if (forkFlag != TActionProcess::isChildProcess()) {
        forkFlag = TActionProcess::isChildProcess();
        reopen(); // re-connect
    }

    if (!queue.isEmpty()) {
        queue += log;
        sendFromQueue();

        if (queue.length() > QUEUE_MAX_SIZE) {
            fprintf(stderr, "queue overflow! len:%d  [%s:%d]\n", queue.length(), __FILE__, __LINE__);
            
            // Removes over logs
            while (queue.length() > QUEUE_MAX_SIZE) {
                int i = queue.indexOf('\n');
                if (i >= 0) {
                    queue.remove(0, i + 1);
                } else {
                    queue.clear();
                }
            }
        }

        if (socket->state() == QLocalSocket::UnconnectedState) {
            reopen();
        }

    } else {
        int len = send(log);
        if (len < 0)
            return;
        
        if (len < log.length()) {
            queue.append(log.data() + len, log.length() - len);
            timer.start(1, this);
        } else {
            timer.stop();
        }
    }
}


bool TLogSender::waitForConnected(int msecs)
{
    return socket->waitForConnected(msecs);
}


int TLogSender::send(const QByteArray &log) const
{
    int len = 0;
    if (!log.isEmpty() && socket->state() == QLocalSocket::ConnectedState) {
        len = socket->write(log);
        if (len < 0) {
            fprintf(stderr, "log send failed [%s:%d]\n", __FILE__, __LINE__);
        } else {
            socket->flush();
        }
    }
    return len;
}


void TLogSender::sendFromQueue()
{
    int len = send(queue);
    if (len > 0) {
        queue.remove(0, len);
    }
}


void TLogSender::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        sendFromQueue();
        if (queue.isEmpty()) {
            timer.stop();
        }
    } else {
        QObject::timerEvent(event);
    }
}
