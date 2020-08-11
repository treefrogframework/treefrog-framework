#pragma once
#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDriver>

class TMongoCursor;


class T_CORE_EXPORT TMongoDriver : public TKvsDriver {
public:
    TMongoDriver();
    ~TMongoDriver();

    QString key() const { return QStringLiteral("MONGODB"); }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString &options = QString());
    void close();
    bool isOpen() const;

    bool find(const QString &collection, const QVariantMap &criteria, const QVariantMap &orderBy,
        const QStringList &fields, int limit, int skip);
    QVariantMap findOne(const QString &collection, const QVariantMap &criteria,
        const QStringList &projectFields = QStringList());
    bool insertOne(const QString &collection, const QVariantMap &object, QVariantMap *reply = nullptr);
    bool updateOne(const QString &collection, const QVariantMap &criteria, const QVariantMap &object,
        bool upsert = false, QVariantMap *reply = nullptr);
    bool updateMany(const QString &collection, const QVariantMap &criteria, const QVariantMap &object,
        bool upsert = false, QVariantMap *reply = nullptr);
    bool removeOne(const QString &collection, const QVariantMap &criteria, QVariantMap *reply = nullptr);
    bool removeMany(const QString &collection, const QVariantMap &criteria, QVariantMap *reply = nullptr);
    qint64 count(const QString &collection, const QVariantMap &criteria);
    int lastErrorDomain() const { return errorDomain; }
    int lastErrorCode() const { return errorCode; }
    QString lastErrorString() const { return errorString; }
    QStringList getCollectionNames();
    QString serverVersion();
    int serverVersionNumber();

    TMongoCursor &cursor() { return *mongoCursor; }
    const TMongoCursor &cursor() const { return *mongoCursor; }

private:
    typedef struct _bson_error_t bson_error_t;
    typedef struct _mongoc_client_t mongoc_client_t;

    void clearError();
    void setLastError(const bson_error_t *error);

    mongoc_client_t *mongoClient {nullptr};
    TMongoCursor *mongoCursor {nullptr};
    QString dbName;
    int serverVerionNumber {-1};
    int errorDomain {0};
    int errorCode {0};
    QString errorString;

    T_DISABLE_COPY(TMongoDriver)
    T_DISABLE_MOVE(TMongoDriver)
};

