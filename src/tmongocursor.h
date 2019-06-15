#ifndef TMONGOCURSOR_H
#define TMONGOCURSOR_H

#include <QVariant>
#include <TGlobal>
#include <TBson>


class T_CORE_EXPORT TMongoCursor
{
public:
    ~TMongoCursor();

    bool next();
    QVariantMap value() const;
    QVariantList toList();

protected:
    typedef void TCursorObject;

    void release();
    TCursorObject *cursor() { return mongoCursor; }
    void setCursor(void *cursor);

private:
    typedef struct _mongoc_cursor_t mongoc_cursor_t;
    mongoc_cursor_t *mongoCursor {nullptr};
    const TBsonObject *bsonDoc {nullptr};  // pointer to a object of bson_t

    TMongoCursor();
    friend class TMongoDriver;
    T_DISABLE_COPY(TMongoCursor)
    T_DISABLE_MOVE(TMongoCursor)
};

#endif // TMONGOCURSOR_H
