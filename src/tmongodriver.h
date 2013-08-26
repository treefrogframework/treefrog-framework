#ifndef TMONGODRIVER_H
#define TMONGODRIVER_H

#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDriver>

struct mongo;
class TMongoCursor;


class T_CORE_EXPORT TMongoDriver : public TKvsDriver
{
public:
    TMongoDriver();
    ~TMongoDriver();

    QString key() const { return "MONGODB"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString & options = QString());
    void close();
    bool isOpen() const;

    int find(const QString &ns, const QVariantMap &criteria, const QVariantMap &orderBy,
             const QStringList &fields, int limit, int skip, int options);
    QVariantMap findOne(const QString &ns, const QVariantMap &criteria,
                        const QStringList &fields = QStringList());
    bool insert(const QString &ns, const QVariantMap &object);
    bool remove(const QString &ns, const QVariantMap &object);
    bool update(const QString &ns, const QVariantMap &criteria,
                const QVariantMap &object, bool upsert = false);
    bool updateMulti(const QString &ns, const QVariantMap &criteria,
                     const QVariantMap &object);
    int count(const QString &ns, const QVariantMap &criteria);
    int lastErrorCode() const;
    QString lastErrorString() const;
    QVariantMap getLastCommandStatus(const QString &db);

    TMongoCursor &cursor() { return *mongoCursor; }
    const TMongoCursor &cursor() const { return *mongoCursor; }

private:
    struct mongo *mongoConnection;
    TMongoCursor *mongoCursor;

    Q_DISABLE_COPY(TMongoDriver)
};

#endif // TMONGODRIVER_H
