#ifndef TDATABASECONTEXTTHREAD_H
#define TDATABASECONTEXTTHREAD_H

#include <TGlobal>
#include <TDatabaseContext>
#include <QThread>


class T_CORE_EXPORT TDatabaseContextThread : public QThread, public TDatabaseContext
{
public:
    TDatabaseContextThread(QObject *parent = nullptr) : QThread(parent), TDatabaseContext() { }
    ~TDatabaseContextThread() = default;
    virtual void run() override;

    T_DISABLE_COPY(TDatabaseContextThread)
    T_DISABLE_MOVE(TDatabaseContextThread)
};

#endif // TDATABASECONTEXTTHREAD_H
