#ifndef TSQLITEBLOBSTORE_H
#define TSQLITEBLOBSTORE_H

#include <TGlobal>
#include <QSqlDatabase>
#include "tcachestore.h"


class T_CORE_EXPORT TSQLiteBlobStore : public TCacheStore
{
public:
    TSQLiteBlobStore(const QString &fileName);
    ~TSQLiteBlobStore() { close(); }

    bool open() override;
    void close() override;
    bool isOpen() const override { return _db.isOpen(); }

    int count() const override;
    bool exists(const QByteArray &name) const override;
    bool read(const QByteArray &name, QByteArray &blob, qint64 &timestamp) override;
    bool write(const QByteArray &name, const QByteArray &blob, qint64 timestamp) override;
    int remove(const QByteArray &name) override;
    int removeOlder(int itemCount);
    int removeOlderThan(qint64 timestamp);
    int removeAll() override;
    bool vacuum();

    static bool setup(const QByteArray &fileName);

private:
    bool exec(const QString &sql);

    QString _dbFile;
    QString _connectionName;
    QSqlDatabase _db;
};

#endif // TSQLITEBLOBSTORE_H
