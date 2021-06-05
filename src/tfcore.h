#pragma once
#include <TWebApplication>
#include <TAbstractController>
#include <TAbstractActionContext>
#include <TAtomic>
#include <TAccessLog>
#include <QtGlobal>
#include <cerrno>
#include <cstdio>
#include <cstring>

#define TF_EINTR_LOOP(func)              \
    int ret;                             \
    do {                                 \
        errno = 0;                       \
        ret = (func);                    \
    } while (ret < 0 && errno == EINTR); \
    return ret;

#define TF_EAGAIN_LOOP(func)                                  \
    int ret;                                                  \
    do {                                                      \
        errno = 0;                                            \
        ret = (func);                                         \
    } while (ret < 0 && (errno == EINTR || errno == EAGAIN)); \
    return ret;


#ifdef Q_OS_UNIX
#include "tfcore_unix.h"
#endif
#ifdef Q_OS_WIN
#include "tfcore_win.h"
#endif
