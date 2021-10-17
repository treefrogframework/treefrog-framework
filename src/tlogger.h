#pragma once
#include <QString>
#include <QVariant>
#include <TGlobal>
#include <TLog>
#if QT_VERSION >= 0x060000
# include <QStringEncoder>
#endif

class TLog;
class QTextCodec;


class T_CORE_EXPORT TLogger {
public:
    TLogger();
    virtual ~TLogger() { }
    virtual QString key() const = 0;
    virtual bool isMultiProcessSafe() const = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void log(const TLog &log) = 0;  // thread safe log output
    virtual void log(const QByteArray &) { }  // thread safe log output
    virtual void flush() { }
    virtual QByteArray logToByteArray(const TLog &log) const;

    const QByteArray &layout() const;
    const QByteArray &dateTimeFormat() const;
    Tf::LogPriority threshold() const;
    const QString &target() const;

#if QT_VERSION < 0x060000
    static QByteArray logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QTextCodec *codec = nullptr);
#else
    static QByteArray logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QStringConverter::Encoding encoding = QStringConverter::Utf8);
#endif
    static QByteArray priorityToString(Tf::LogPriority priority);

protected:
#if QT_VERSION < 0x060000
    QTextCodec *codec() const;
#else
    QStringConverter::Encoding encoding() const;
#endif
    QVariant settingsValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    mutable QByteArray _layout;
    mutable QByteArray _dateTimeFormat;
    mutable Tf::LogPriority _threshold {(Tf::LogPriority)-1};
    mutable QString _target;
#if QT_VERSION < 0x060000
    mutable QTextCodec *_codec {nullptr};
#else
    mutable std::optional<QStringConverter::Encoding> _encoding;
#endif
};
