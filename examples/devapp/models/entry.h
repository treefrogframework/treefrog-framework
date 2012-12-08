#ifndef ENTRY_H
#define ENTRY_H

#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QSharedDataPointer>
#include <TGlobal>
#include <TAbstractModel>

class TSqlObject;
class EntryObject;


class T_MODEL_EXPORT Entry : public TAbstractModel
{
public:
    Entry();
    Entry(const Entry &other);
    Entry(const EntryObject &object);
    ~Entry();

    int id() const;
    void setId(int id);
    QString name() const;
    void setName(const QString &name);
    QString address() const;
    void setAddress(const QString &address);
    QDateTime createdAt() const;
    QDateTime updatedAt() const;
    int revision() const;
    void setRevision(int revision);
    Entry &operator=(const Entry &other);

    static Entry create(int id, const QString &name, const QString &address, int revision);
    static Entry create(const QVariantHash &values);
    static Entry get(int id);
    static QList<Entry> getAll();

private:
    QSharedDataPointer<EntryObject> d;

    TSqlObject *data();
    const TSqlObject *data() const;
};

Q_DECLARE_METATYPE(Entry)
Q_DECLARE_METATYPE(QList<Entry>)

#endif // ENTRY_H
