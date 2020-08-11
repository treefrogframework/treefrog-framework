#pragma once
#include "tcachestore.h"
#include <QSqlDatabase>
#include <TGlobal>


class T_CORE_EXPORT TCacheSQLiteStore : public TCacheStore {
public:
    virtual ~TCacheSQLiteStore();

    QString key() const override { return QLatin1String("sqlite"); }
    DbType dbType() const override { return SQL; }
    bool open() override;
    void close() override;
    bool isOpen() const;

    QByteArray get(const QByteArray &key) override;
    bool set(const QByteArray &key, const QByteArray &value, int seconds) override;
    bool remove(const QByteArray &key) override;
    void clear() override;
    void gc() override;
    QMap<QString, QVariant> defaultSettings() const override;

    bool exists(const QByteArray &key);
    int count();
    bool read(const QByteArray &key, QByteArray &blob, qint64 &timestamp);
    bool write(const QByteArray &key, const QByteArray &blob, qint64 timestamp);
    int removeOlder(int itemCount);
    int removeOlderThan(qint64 timestamp);
    int removeAll();
    //bool vacuum();  // call outside of a transaction
    qint64 dbSize();

    static bool createTable(const QString &table);

protected:
    TCacheSQLiteStore(const QByteArray &table = QByteArray());

    QString _table;

    friend class TCacheFactory;
    friend class TSessionFileDbStore;
};

