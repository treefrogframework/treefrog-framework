#ifndef ENTRYNAME_H
#define ENTRYNAME_H

#include <QString>
#include <QDateTime>
#include <QSharedDataPointer>
#include <TGlobal>
#include <TAbstractModel>

class TSqlObject;
class EntryNameObject;


class T_MODEL_EXPORT EntryName : public TAbstractModel
{
public:
    EntryName();
    EntryName(const EntryName &other);
    EntryName(const EntryNameObject &object);
    ~EntryName();

    int idIndex() const;
    void setIdIndex(int idIndex);
    QString fullName() const;
    void setFullName(const QString &fullName);
    QString address() const;
    void setAddress(const QString &address);
    QDateTime createdAt() const;
    int entryNumber() const;
    void setEntryNumber(int entryNumber);

    static EntryName create(int idIndex, const QString &fullName, const QString &address, int entryNumber);
    static EntryName get(int idIndex, const QString &fullName, const QString &address, int entryNumber);

private:
    QSharedDataPointer<EntryNameObject> d;

    TSqlObject *data();
    const TSqlObject *data() const;
};

Q_DECLARE_METATYPE(EntryName)

#endif // ENTRYNAME_H
