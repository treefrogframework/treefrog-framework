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
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), uint16_t port = 0, const QString &options = QString());
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
    int64_t count(const QString &collection, const QVariantMap &criteria);
    int lastErrorDomain() const { return _errorDomain; }
    int lastErrorCode() const { return _errorCode; }
    QString lastErrorString() const { return _errorString; }
    QStringList getCollectionNames();
    QString serverVersion();
    int serverVersionNumber();

    TMongoCursor &cursor() { return *_mongoCursor; }
    const TMongoCursor &cursor() const { return *_mongoCursor; }

private:
    void clearError();
    void setLastError(const void *error);

    void *_mongoClient {nullptr};
    TMongoCursor *_mongoCursor {nullptr};
    QString _dbName;
    int _serverVerionNumber {-1};
    int _errorDomain {0};
    int _errorCode {0};
    QString _errorString;

    T_DISABLE_COPY(TMongoDriver)
    T_DISABLE_MOVE(TMongoDriver)
};
