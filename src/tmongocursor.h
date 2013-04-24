#ifndef TMONGOCURSOR_H
#define TMONGOCURSOR_H

#include <QVariant>
#include <TGlobal>

typedef void TMongoCursorObject;
struct mongo;


class T_CORE_EXPORT TMongoCursor
{
public:
    ~TMongoCursor();

    bool next();
    QVariantMap value() const;
    QVariantList toList();

protected:
    void init(mongo *connection, const QString &ns);
    void release();
    TMongoCursorObject *cursor() { return mongoCursor; }
    void setCursor(TMongoCursorObject *cursor);

private:
    TMongoCursorObject *mongoCursor;  // pointer to object of struct mongo_cursor

    TMongoCursor();
    friend class TMongoDriver;
    Q_DISABLE_COPY(TMongoCursor)
};

#endif // TMONGOCURSOR_H
