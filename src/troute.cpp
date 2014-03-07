#include "troute.h"
#include "QHash"
#include <TGlobal>  // For Q_GLOBAL_STATIC_WITH_INITIALIZER

typedef QHash<QString, int> String2MethodHash;
typedef QHash<int, QString> Method2StringHash;


Q_GLOBAL_STATIC_WITH_INITIALIZER(String2MethodHash, String2Method,
{
    x->insert("MATCH", TRoute::Match);
    x->insert("GET",   TRoute::Get);
    x->insert("POST",  TRoute::Post);
});

Q_GLOBAL_STATIC_WITH_INITIALIZER(Method2StringHash, Method2String,
{
    x->insert(TRoute::Match, "MATCH");
    x->insert(TRoute::Get,   "GET");
    x->insert(TRoute::Post,  "POST");
});


int TRoute::methodFromString(QString name)
{
    return String2Method()->value(name.toUpper(), TRoute::Invalid);
}

QString TRoute::methodToString(int method)
{
    return Method2String()->value(method);
}