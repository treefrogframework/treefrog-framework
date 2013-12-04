#ifndef BLOG_H
#define BLOG_H

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>
#include <TGlobal>
#include <TAbstractModel>

class TModelObject;
class BlogObject;


class T_MODEL_EXPORT Blog : public TAbstractModel
{
public:
    Blog();
    Blog(const Blog &other);
    Blog(const BlogObject &object);
    ~Blog();

    int id() const;
    QString title() const;
    void setTitle(const QString &title);
    QString body() const;
    void setBody(const QString &body);
    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    int lockRevision() const;
    Blog &operator=(const Blog &other);

    bool create() { return TAbstractModel::create(); }
    bool update() { return TAbstractModel::update(); }
    bool save()   { return TAbstractModel::save(); }
    bool remove() { return TAbstractModel::remove(); }

    static Blog create(const QString &title, const QString &body);
    static Blog create(const QVariantMap &values);
    static Blog get(int id);
    static Blog get(int id, int lockRevision);
    static int count();
    static QList<Blog> getAll();

private:
    QSharedDataPointer<BlogObject> d;

    TModelObject *modelData();
    const TModelObject *modelData() const;
};

Q_DECLARE_METATYPE(Blog)
Q_DECLARE_METATYPE(QList<Blog>)

#endif // BLOG_H
