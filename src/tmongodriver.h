#ifndef TMONGODRIVER_H
#define TMONGODRIVER_H

#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDriver>

class TMongoCursor;
class TBson;


class T_CORE_EXPORT TMongoDriver : public TKvsDriver
{
public:
    TMongoDriver();
    ~TMongoDriver();

    QString key() const { return "MONGODB"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString & options = QString());
    void close();
    bool isOpen() const;

    bool find(const QString &collection, const QVariantMap &criteria, const QVariantMap &orderBy,
              const QStringList &fields, int limit, int skip, int options);
    QVariantMap findOne(const QString &collection, const QVariantMap &criteria,
                        const QStringList &fields = QStringList());
    bool insert(const QString &collection, const QVariantMap &object);
    bool remove(const QString &collection, const QVariantMap &object);
    bool update(const QString &collection, const QVariantMap &criteria,
                const QVariantMap &object, bool upsert = false);
    bool updateMulti(const QString &collection, const QVariantMap &criteria,
                     const QVariantMap &object);
    int count(const QString &collection, const QVariantMap &criteria);
    int lastErrorCode() const { return errorCode; }
    QString lastErrorString() const { return errorString; }
    QVariantMap getLastCommandStatus() const;

    TMongoCursor &cursor() { return *mongoCursor; }
    const TMongoCursor &cursor() const { return *mongoCursor; }

private:
    void setLastCommandStatus(const void *bson);

    typedef struct _mongoc_client_t mongoc_client_t;
    mongoc_client_t *mongoClient {nullptr};
    QString dbName;
    TMongoCursor *mongoCursor {nullptr};
    TBson *lastStatus {nullptr};
    int errorCode {0};
    QString errorString;

    T_DISABLE_COPY(TMongoDriver)
    T_DISABLE_MOVE(TMongoDriver)
};

#endif // TMONGODRIVER_H
