#ifndef TCACHEMONGOSTORE_H
#define TCACHEMONGOSTORE_H

#include <TGlobal>
#include "tcachestore.h"


class T_CORE_EXPORT TCacheMongoStore : public TCacheStore
{
public:
    virtual ~TCacheMongoStore() {}

    QString key() const { return QLatin1String("mongodb"); }
    bool open() override;
    void close() override;

    QByteArray get(const QByteArray &key) override;
    bool set(const QByteArray &key, const QByteArray &value, int seconds) override;
    bool remove(const QByteArray &key) override;
    void clear() override;
    void gc() override;
    QMap<QString, QVariant> defaultSettings() const override;

protected:
    TCacheMongoStore();

    friend class TCacheFactory;
};

#endif // TCACHEMONGOSTORE_H
