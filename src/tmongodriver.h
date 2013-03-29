#ifndef TMONGODRIVER_H
#define TMONGODRIVER_H

#include <QStringList>
#include <QVariant>
#include <TGlobal>

struct mongo;
class TMongoCursor;


class T_CORE_EXPORT TMongoDriver
{
public:
    TMongoDriver();
    ~TMongoDriver();

    bool open(const QString &host);
    void close();
    bool isOpen() const;
    
    bool find(const QString &ns, const QVariantMap &query,
              const QStringList &fields, int limit, int skip, int options);
    QVariantMap findFirst(const QString &ns, const QVariantMap &query,
                          const QStringList &fields = QStringList());
    bool insert(const QString &ns, const QVariantMap &object);
    bool remove(const QString &ns, const QVariantMap &object);
    bool update(const QString &ns, const QVariantMap &query,
                const QVariantMap &object, bool upsert = false);
    bool updateMulti(const QString &ns, const QVariantMap &query,
                     const QVariantMap &object, bool upsert = false);

    TMongoCursor &cursor() { return *mongoCursor; }
    const TMongoCursor &cursor() const { return *mongoCursor; }

private:
    struct mongo *mongoConnection;
    TMongoCursor *mongoCursor;
    
    Q_DISABLE_COPY(TMongoDriver)
};

#endif // TMONGODRIVER_H
