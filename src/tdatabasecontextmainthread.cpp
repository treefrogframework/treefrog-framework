#include "tdatabasecontextmainthread.h"


TDatabaseContextMainThread::TDatabaseContextMainThread(QObject *parent)
    : TDatabaseContextThread(parent)
{
    setTransactionEnabled(false);
}
