#ifndef FOO_H
#define FOO_H

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>
#include <TGlobal>
#include <TAbstractModel>

class TModelObject;
class FooObject;
class QJsonArray;


class T_MODEL_EXPORT Foo : public TAbstractModel
{
public:
    Foo();
    Foo(const Foo &other);
    Foo(const FooObject &object);
    ~Foo();

    QString id() const;
    QString title() const;
    void setTitle(const QString &title);
    QString body() const;
    void setBody(const QString &body);
    int length() const;
    void setLength(int length);
    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    int lockRevision() const;
    Foo &operator=(const Foo &other);

    bool create() { return TAbstractModel::create(); }
    bool update() { return TAbstractModel::update(); }
    bool save()   { return TAbstractModel::save(); }
    bool remove() { return TAbstractModel::remove(); }

    static Foo create(const QString &title, const QString &body, int length);
    static Foo create(const QVariantMap &values);
    static Foo get(const QString &id);
    static Foo get(const QString &id, int lockRevision);
    static int count();
    static QList<Foo> getAll();
#if QT_VERSION >= 0x050000
    static QJsonArray getAllJson();
#endif

private:
    QSharedDataPointer<FooObject> d;

    TModelObject *modelData();
    const TModelObject *modelData() const;
};

Q_DECLARE_METATYPE(Foo)
Q_DECLARE_METATYPE(QList<Foo>)

#endif // FOO_H
