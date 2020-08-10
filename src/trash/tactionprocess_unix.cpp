/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTimerEvent>
#include <QCoreApplication>
#include <TActionProcess>
#include "tfcore_unix.h"


void TActionProcessManager::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    // cleanup resources of child processes
    for (;;) {
        int status = 0;
        pid_t pid = tf_waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            TActionProcess *p = value(pid, 0);
            if (p) {
                remove(pid);

                if (WIFEXITED(status)) {
                    p->terminate(WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    p->kill(WTERMSIG(status));
                } else {
                    tWarn("Invalid status infomation of child process: %d", status);
                    p->kill(-1);
                }
            } else {
                tError("wait4 pid:%d, not found such TActionProcess object", pid);
            }

        } else {
            break;
        }
    }
}


void TActionProcess::start()
{
    if (childPid > 0) {
        tWarn("forked already");
        return;
    }

    if (isChildProcess()) {
        tError("start failed, not parent process");
        return;
    }

    childPid = fork();
    int lastForkErrno = errno;
    if (childPid == 0) {
        // Starts the child
        Q_ASSERT(currentActionProcess == 0);
        currentActionProcess = this;
        TActionProcessManager::instance()->clear();

        emit forked();
        execute();
        emit finished();
        QCoreApplication::exit(1);

    } else if (childPid > 0) {
        // parent process
        tSystemDebug("fork succeeded. pid: %d", childPid);
        TActionProcessManager::instance()->insert(childPid, this);
        tf_close_socket(TActionContext::socketDescriptor());
        emit started();
    } else {
        tFatal("fork failed: %s", qPrintable(qt_error_string(lastForkErrno)));
        QCoreApplication::exit(-1);
    }
}
