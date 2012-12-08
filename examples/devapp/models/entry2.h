#ifndef ENTRY2_H
#define ENTRY2_H

#include "TLazyLoader"

class EntryObject;


class T_MODEL_EXPORT Entry2 : public TLazyLoader<EntryObject, int>
{
public:
    Entry2();
    Entry2(const EntryObject &entry);
    virtual ~Entry2();

    int id();
    void setId(int id);
    QString name();
    void setName(const QString &name);
    QString address();
    void setAddress(const QString &address);
    
    bool save();
    bool update();
    bool remove();

    static Entry2 load(int id);
    static Entry2 get(int id);

//     static bool update(int id, const Entry2 &entry);
//     static bool remove(const Entry2 &entry);
//     static bool remove(int id);

    /* Test */
    static void proc();
    static void hoge();

protected:
    EntryObject *lazyObject() { return sqlObject; }

private:
    Entry2 &operator=(const Entry2 &entry);
    EntryObject *sqlObject;
};

#endif // ENTRY2_H
