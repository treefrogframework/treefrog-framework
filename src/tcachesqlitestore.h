#ifndef TCACHESQLITESTORE_H
#define TCACHESQLITESTORE_H

#include <TGlobal>
#include <QSqlDatabase>
#include "tcachestore.h"


class T_CORE_EXPORT TCacheSQLiteStore : public TCacheStore
{
public:
    virtual ~TCacheSQLiteStore();

    QString key() const { return QLatin1String("singlefiledb"); }
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
    bool vacuum();
    qint64 dbSize();

    static bool createTable(const QString &table);

protected:
    TCacheSQLiteStore(qint64 thresholdFileSize = 0, const QByteArray &table = QByteArray());

    qint64 _thresholdFileSize {0};
    QString _table;

    friend class TCacheFactory;
    friend class TSessionFileDbStore;
};

#endif // TCACHESQLITESTORE_H
