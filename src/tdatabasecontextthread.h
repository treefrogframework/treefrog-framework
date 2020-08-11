#pragma once
#include <QThread>
#include <TDatabaseContext>
#include <TGlobal>


class T_CORE_EXPORT TDatabaseContextThread : public QThread, public TDatabaseContext {
public:
    TDatabaseContextThread(QObject *parent = nullptr) :
        QThread(parent), TDatabaseContext() { }
    ~TDatabaseContextThread() = default;
    virtual void run() override;

    T_DISABLE_COPY(TDatabaseContextThread)
    T_DISABLE_MOVE(TDatabaseContextThread)
};

