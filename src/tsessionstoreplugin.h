#pragma once
#include <QObject>
#include <QStringList>
#include <QtPlugin>
#include <TGlobal>

class TSessionStore;

constexpr auto TSessionStoreInterface_iid = "org.treefrogframework.TreeFrog.TSessionStoreInterface/2.0";


class T_CORE_EXPORT TSessionStoreInterface {
public:
    virtual ~TSessionStoreInterface() { }
    virtual TSessionStore *create(const QString &key) = 0;
    virtual void destroy(const QString &key, TSessionStore *store) = 0;
};

Q_DECLARE_INTERFACE(TSessionStoreInterface, TSessionStoreInterface_iid)


class T_CORE_EXPORT TSessionStorePlugin : public QObject, public TSessionStoreInterface {
    Q_OBJECT
    Q_INTERFACES(TSessionStoreInterface)

public:
    explicit TSessionStorePlugin(QObject *parent = 0) :
        QObject(parent) { }
    ~TSessionStorePlugin() { }

    virtual TSessionStore *create(const QString &key) = 0;
    virtual void destroy(const QString &key, TSessionStore *store) = 0;
};

