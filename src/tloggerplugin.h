#ifndef TLOGGERPLUGIN_H
#define TLOGGERPLUGIN_H

#include <QObject>
#include <QStringList>
#include <TGlobal>

class TLogger;


class T_CORE_EXPORT TLoggerInterface
{
public:
    virtual ~TLoggerInterface() { }
    virtual QStringList keys() const = 0;
    virtual TLogger *create(const QString &key) = 0;
};

Q_DECLARE_INTERFACE(TLoggerInterface, "TLoggerInterface/1.0")


class T_CORE_EXPORT TLoggerPlugin : public QObject, public TLoggerInterface
{
    Q_OBJECT
    Q_INTERFACES(TLoggerInterface)

public:
    explicit TLoggerPlugin(QObject *parent = 0) : QObject(parent) { }
    ~TLoggerPlugin() { }

    virtual QStringList keys() const = 0;
    virtual TLogger *create(const QString &key) = 0;
};

#endif // TLOGGERPLUGIN_H
