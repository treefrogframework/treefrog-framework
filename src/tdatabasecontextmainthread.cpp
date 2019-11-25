#include "tdatabasecontextmainthread.h"


TDatabaseContextMainThread::TDatabaseContextMainThread(QObject *parent) :
    TDatabaseContextThread(parent)
{
    for (int databaseId = 0; databaseId < Tf::app()->sqlDatabaseSettingsCount(); ++databaseId) {
        setTransactionEnabled(false, databaseId);
    }
}
