#ifndef TSQLITEBLOBSTORE_H
#define TSQLITEBLOBSTORE_H

#include <TGlobal>
#include <QByteArray>
#include <QSqlDatabase>


class T_CORE_EXPORT TSQLiteBlobStore
{
public:
    TSQLiteBlobStore(const QString &fileName);
    ~TSQLiteBlobStore() { close(); }

    bool open();
    void close();
    bool isOpen() const { return _db.isOpen(); }

    int count() const;
    bool exists(const QByteArray &name) const;
    bool read(const QByteArray &name, QByteArray &blob, qint64 &timestamp);
    bool write(const QByteArray &name, const QByteArray &blob, qint64 timestamp);
    int remove(const QByteArray &name);
    int removeOlder(int itemCount);
    int removeOlderThan(qint64 timestamp);
    int removeAll();
    bool vacuum();

    static bool setup(const QByteArray &fileName);

private:
    bool exec(const QString &sql);

    QString _dbFile;
    QString _connectionName;
    QSqlDatabase _db;
};

#endif // TSQLITEBLOBSTORE_H
