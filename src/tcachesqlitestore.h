#ifndef TCACHESQLITESTORE_H
#define TCACHESQLITESTORE_H

#include <TGlobal>
#include <QSqlDatabase>
#include "tcachestore.h"


class T_CORE_EXPORT TCacheSQLiteStore : public TCacheStore
{
public:
    TCacheSQLiteStore() {}
    TCacheSQLiteStore(const QString &fileName, const QString &connectOptions = QString(), qint64 thresholdFileSize = 0);
    virtual ~TCacheSQLiteStore() { close(); }

    QString key() const { return QStringLiteral("singlefiledb"); }
    bool open() override;
    void close() override;
    bool isOpen() const;

    QByteArray get(const QByteArray &key) override;
    bool set(const QByteArray &key, const QByteArray &value, qint64 msecs) override;
    bool remove(const QByteArray &key) override;
    void clear() override;
    void gc() override;

    bool exists(const QByteArray &key) const;
    int count() const;
    bool read(const QByteArray &key, QByteArray &blob, qint64 &timestamp);
    bool write(const QByteArray &key, const QByteArray &blob, qint64 timestamp);
    int removeOlder(int itemCount);
    int removeOlderThan(qint64 timestamp);
    int removeAll();
    bool vacuum();

protected:
    qint64 fileSize() const;
    bool exec(const QString &sql);

    QString _dbFile;
    QString _connectOptions;
    qint64 _thresholdFileSize {0};
    QString _connectionName;
    QSqlDatabase _db;
};

#endif // TCACHESQLITESTORE_H
