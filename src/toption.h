#ifndef TOPTION_H
#define TOPTION_H

#include <QMap>
#include <QVariant>
#include <TGlobal>

typedef QMap<int, QVariant> TOption;

TOption T_CORE_EXPORT operator|(const TOption &o1, const TOption &o2);


#endif // TOPTION_H
