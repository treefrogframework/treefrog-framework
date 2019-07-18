#ifndef TSQLITEBLOBSTORE_H
#define TSQLITEBLOBSTORE_H

#include <TGlobal>
#include <QByteArray>

typedef struct sqlite3 sqlite3;


class T_CORE_EXPORT TSQLiteBlobStore
{
public:
    TSQLiteBlobStore() {}
    ~TSQLiteBlobStore() { close(); }

    bool open(const QByteArray &fileName);
    void close();
    bool isOpen() const { return (bool)_db; }

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
    sqlite3 *_db  {nullptr};
};

#endif // TSQLITEBLOBSTORE_H
