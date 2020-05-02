/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSystemGlobal>
#include <TWebApplication>
#include <cerrno>
#include <csignal>

namespace {
volatile sig_atomic_t unixSignal = -1;

void signalHandler(int signum)
{
    unixSignal = signum;
}
}


void TWebApplication::watchUnixSignal(int sig, bool watch)
{
    if (sig < NSIG) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_RESTART;
        if (watch) {
            sa.sa_handler = signalHandler;
            _timer.start(500, this);
        } else {
            sa.sa_handler = SIG_DFL;
        }

        if (sigaction(sig, &sa, 0) != 0) {
            tSystemError("sigaction failed  errno:%d", errno);
        }
    }
}


void TWebApplication::ignoreUnixSignal(int sig, bool ignore)
{
    if (sig < NSIG) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = (ignore) ? SIG_IGN : SIG_DFL;
        if (sigaction(sig, &sa, 0) != 0) {
            tSystemError("sigaction failed  errno:%d", errno);
        }
    }
}


int TWebApplication::signalNumber()
{
    return ::unixSignal;
}


void TWebApplication::resetSignalNumber()
{
    ::unixSignal = -1;
}
