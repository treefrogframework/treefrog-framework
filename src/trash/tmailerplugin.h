#ifndef TMAILERPLUGIN_H
#define TMAILERPLUGIN_H

#include <QObject>
#include <QStringList>
#include <TGlobal>

class TMailer;


class T_CORE_EXPORT TMailerInterface
{
public:
    virtual ~TMailerInterface() { }
    virtual QStringList keys() const = 0;
    virtual TMailer *create(const QString &key) = 0;
};

Q_DECLARE_INTERFACE(TMailerInterface, "TMailerInterface/1.0")


class T_CORE_EXPORT TMailerPlugin : public QObject, public TMailerInterface
{
    Q_OBJECT
    Q_INTERFACES(TMailerInterface)

public:
    explicit TMailerPlugin(QObject *parent = 0) : QObject(parent) { }
    ~TMailerPlugin() { }

    virtual QStringList keys() const = 0;
    virtual TMailer *create(const QString &key) = 0;
};

#endif // TMAILERPLUGIN_H
