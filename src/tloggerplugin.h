#pragma once
#include <QObject>
#include <QStringList>
#include <QtPlugin>
#include <TGlobal>

class TLogger;

constexpr auto TLoggerInterface_iid = "org.treefrogframework.TreeFrog.TLoggerInterface/1.0";


class T_CORE_EXPORT TLoggerInterface {
public:
    virtual ~TLoggerInterface() { }
    virtual TLogger *create(const QString &key) = 0;
};

Q_DECLARE_INTERFACE(TLoggerInterface, TLoggerInterface_iid)


class T_CORE_EXPORT TLoggerPlugin : public QObject, public TLoggerInterface {
    Q_OBJECT
    Q_INTERFACES(TLoggerInterface)

public:
    explicit TLoggerPlugin(QObject *parent = 0) :
        QObject(parent) { }
    ~TLoggerPlugin() { }

    virtual TLogger *create(const QString &key) = 0;
};

