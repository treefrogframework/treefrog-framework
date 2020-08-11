#pragma once
#include <QByteArray>
#include <QString>
#include <TGlobal>

class TLogger;


class T_CORE_EXPORT TAccessLogStream {
public:
    TAccessLogStream(const QString &fileName);
    ~TAccessLogStream();
    void writeLog(const QByteArray &log);
    void flush();

private:
    TLogger *logger {nullptr};

    // Disable
    TAccessLogStream();
    T_DISABLE_COPY(TAccessLogStream)
    T_DISABLE_MOVE(TAccessLogStream)
};

