#pragma once
#include "tfnamespace.h"
#include <QString>
#include <QTextStream>
#include <QtCore>
#include <TGlobal>

namespace Tf {
T_CORE_EXPORT void setupAppLoggers();  // internal use
T_CORE_EXPORT void releaseAppLoggers();  // internal use
}


class T_CORE_EXPORT TDebug {
public:
    TDebug(int priority) :
        msgPriority(priority) {}
    TDebug(const TDebug &other);
    ~TDebug();
    TDebug &operator=(const TDebug &other);

    inline TDebug fatal() const { return TDebug(Tf::FatalLevel); }
    inline TDebug error() const { return TDebug(Tf::ErrorLevel); }
    inline TDebug warn() const { return TDebug(Tf::WarnLevel); }
    inline TDebug info() const { return TDebug(Tf::InfoLevel); }
    inline TDebug debug() const { return TDebug(Tf::DebugLevel); }
    inline TDebug trace() const { return TDebug(Tf::TraceLevel); }

    void fatal(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);
    void error(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);
    void warn(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);
    void info(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);
    void debug(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);
    void trace(const char *fmt, ...) const T_ATTRIBUTE_FORMAT(2, 3);

    inline TDebug &operator<<(QChar t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(bool t)
    {
        ts << (t ? "true" : "false");
        return *this;
    }
    inline TDebug &operator<<(char t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(short t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(unsigned short t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(int t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(unsigned int t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(long t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(unsigned long t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(qint64 t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(quint64 t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(float t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(double t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(const char *t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(const QString &t)
    {
        ts << t;
        return *this;
    }
#if QT_VERSION < 0x060000
    inline TDebug &operator<<(const QStringRef &t)
    {
        ts << t.toString();
        return *this;
    }
#endif
    inline TDebug &operator<<(const QLatin1String &t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(const QStringList &t)
    {
        QString str;
        for (auto &s : t) {
            str += "\"";
            str += s;
            str += "\", ";
        }
        str.chop(2);
        ts << "[" << str << "]";
        return *this;
    }
    inline TDebug &operator<<(const QByteArray &t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(const QByteArrayList &t)
    {
        QByteArray str;
        for (auto &s : t) {
            str += "\"";
            str += s;
            str += "\", ";
        }
        str.chop(2);
        ts << '[' << str << ']';
        return *this;
    }
    inline TDebug &operator<<(const QVariant &t)
    {
        ts << t.toString();
        return *this;
    }
    inline TDebug &operator<<(const void *t)
    {
        ts << t;
        return *this;
    }
    inline TDebug &operator<<(std::nullptr_t)
    {
        ts << "(nullptr)";
        return *this;
    }

private:
    QString buffer;
    QTextStream ts {&buffer, Tf::WriteOnly};
    int msgPriority {0};
};

