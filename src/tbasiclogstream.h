#pragma once
#include "tabstractlogstream.h"
#include <QMutex>


class T_CORE_EXPORT TBasicLogStream : public TAbstractLogStream {
public:
    TBasicLogStream(const QList<TLogger *> loggers);
    ~TBasicLogStream();

    void writeLog(const TLog &log) override;
    void flush() override;

private:
    QMutex mutex;

    T_DISABLE_COPY(TBasicLogStream)
    T_DISABLE_MOVE(TBasicLogStream)
};
