#pragma once
#include <QString>
#include <QVariant>
#include <TGlobal>
#include <TLog>
#include <QStringEncoder>

class TLog;
class QTextCodec;

namespace Tf {

T_CORE_EXPORT void setAppLogLayout(const QByteArray &layout);
T_CORE_EXPORT void setAppLogDateTimeFormat(const QByteArray &format);

}


class T_CORE_EXPORT TLogger {
public:
    TLogger();
    virtual ~TLogger() { }
    virtual QString key() const = 0;
    virtual bool isMultiProcessSafe() const = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void log(const QByteArray &) = 0;  // thread safe log output
    virtual void log(const TLog &tlog) { log(logToByteArray(tlog)); }  // thread safe log output
    virtual void flush() { }
    virtual QByteArray logToByteArray(const TLog &log) const;

    const QByteArray &layout() const;
    const QByteArray &dateTimeFormat() const;
    Tf::LogPriority threshold() const;
    const QString &target() const;

    static QByteArray logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QStringConverter::Encoding encoding = QStringConverter::Utf8);
    static QByteArray priorityToString(Tf::LogPriority priority);

protected:
    QStringConverter::Encoding encoding() const;
    QVariant settingsValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    mutable Tf::LogPriority _threshold {(Tf::LogPriority)-1};
    mutable QString _target;
    mutable std::optional<QStringConverter::Encoding> _encoding;
};
