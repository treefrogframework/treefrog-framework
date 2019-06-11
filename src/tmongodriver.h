#ifndef TMONGODRIVER_H
#define TMONGODRIVER_H

#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDriver>

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

    bool find(const QString &collection, const QVariantMap &criteria, const QVariantMap &orderBy,
              const QStringList &fields, int limit, int skip, int options);
    QVariantMap findOne(const QString &collection, const QVariantMap &criteria,
                        const QStringList &projectFields = QStringList());
    bool insert(const QString &collection, const QVariantMap &object);
    bool remove(const QString &collection, const QVariantMap &object);
    bool update(const QString &collection, const QVariantMap &criteria,
                const QVariantMap &object, bool upsert = false);
    bool updateMulti(const QString &collection, const QVariantMap &criteria,
                     const QVariantMap &object);
    int count(const QString &collection, const QVariantMap &criteria);
    int lasrErrorDomain() const { return errorDomain; }
    int lastErrorCode() const { return errorCode; }
    QString lastErrorString() const { return errorString; }

    TMongoCursor &cursor() { return *mongoCursor; }
    const TMongoCursor &cursor() const { return *mongoCursor; }

private:
    typedef struct _bson_error_t bson_error_t;
    typedef struct _mongoc_client_t mongoc_client_t;

    void clearError();
    void setLastError(const bson_error_t* error);

    mongoc_client_t *mongoClient {nullptr};
    TMongoCursor *mongoCursor {nullptr};
    QString dbName;
    int errorDomain {0};
    int errorCode {0};
    QString errorString;

    T_DISABLE_COPY(TMongoDriver)
    T_DISABLE_MOVE(TMongoDriver)
};

#endif // TMONGODRIVER_H
