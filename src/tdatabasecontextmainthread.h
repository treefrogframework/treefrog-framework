#ifndef TDATABASECONTEXTMAINTHREAD_H
#define TDATABASECONTEXTMAINTHREAD_H

#include "tdatabasecontextthread.h"


class T_CORE_EXPORT TDatabaseContextMainThread : public TDatabaseContextThread
{
public:
    TDatabaseContextMainThread(QObject *parent = nullptr);
    ~TDatabaseContextMainThread() = default;

    T_DISABLE_COPY(TDatabaseContextMainThread)
    T_DISABLE_MOVE(TDatabaseContextMainThread)
};

#endif // TDATABASECONTEXTMAINTHREAD_H
