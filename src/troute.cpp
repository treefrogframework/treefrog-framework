#include "troute.h"
#include "QHash"
#include <TGlobal>  // For Q_GLOBAL_STATIC_WITH_INITIALIZER

class String2MethodHash : public QHash<QString, int>
{
public:
    String2MethodHash() : QHash<QString, int>()
    {
        insert("MATCH",    TRoute::Match);
        insert("GET",      TRoute::Get);
        insert("POST",     TRoute::Post);
        insert("PUT",      TRoute::Put);
        insert("PATCH",    TRoute::Patch);
        insert("DELETE",   TRoute::Delete);
    }
};

Q_GLOBAL_STATIC(String2MethodHash, String2Method);


class Method2StringHash : public QHash<int, QString>
{
public:
    Method2StringHash() : QHash<int, QString>()
    {
        insert(TRoute::Match,  "MATCH");
        insert(TRoute::Get,    "GET");
        insert(TRoute::Post,   "POST");
        insert(TRoute::Put,    "PUT");
        insert(TRoute::Patch,  "PATCH");
        insert(TRoute::Delete, "DELETE");
    }
};

Q_GLOBAL_STATIC(Method2StringHash, Method2String);



int TRoute::methodFromString(QString name)
{
    return String2Method()->value(name.toUpper(), TRoute::Invalid);
}

QString TRoute::methodToString(int method)
{
    return Method2String()->value(method);
}