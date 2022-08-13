#pragma once
#include <QVariant>
#include <TBson>
#include <TGlobal>


class T_CORE_EXPORT TMongoCursor {
    using TCursorObject = void;
    typedef struct _mongoc_cursor_t mongoc_cursor_t;

public:
    ~TMongoCursor();

    bool next();
    QVariantMap value() const;
    QVariantList toList();

protected:
    void release();
    TCursorObject *cursor() { return _mongoCursor; }
    void setCursor(void *cursor);

private:
    mongoc_cursor_t *_mongoCursor {nullptr};
    const TBsonObject *_bsonDoc {nullptr};  // pointer to a object of bson_t

    friend class TMongoDriver;
    TMongoCursor();
    T_DISABLE_COPY(TMongoCursor)
    T_DISABLE_MOVE(TMongoCursor)
};
