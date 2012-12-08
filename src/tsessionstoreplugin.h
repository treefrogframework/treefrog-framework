#ifndef TSESSIONSTOREPLUGIN_H
#define TSESSIONSTOREPLUGIN_H

#include <QObject>
#include <QStringList>
#include <TGlobal>

class TSessionStore;


class T_CORE_EXPORT TSessionStoreInterface
{
public:
    virtual ~TSessionStoreInterface() { }
    virtual QStringList keys() const = 0;
    virtual TSessionStore *create(const QString &key) = 0;
};

Q_DECLARE_INTERFACE(TSessionStoreInterface, "TSessionStoreInterface/1.0")


class T_CORE_EXPORT TSessionStorePlugin : public QObject, public TSessionStoreInterface
{
    Q_OBJECT
    Q_INTERFACES(TSessionStoreInterface)

public:
    explicit TSessionStorePlugin(QObject *parent = 0) : QObject(parent) { }
    ~TSessionStorePlugin() { }

    virtual QStringList keys() const = 0;
    virtual TSessionStore *create(const QString &key) = 0;
};

#endif // TSESSIONSTOREPLUGIN_H
