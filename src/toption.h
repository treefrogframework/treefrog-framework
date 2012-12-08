#ifndef TOPTION_H
#define TOPTION_H

#include <QHash>
#include <QVariant>
#include <TGlobal>


#if 0
class T_CORE_EXPORT TOption : public QHash<int, QVariant>
{
public:
    TOption() { }
    TOption(const TOption &other);
    TOption(const QHash<int, QVariant> hash);
    TOption &operator=(const TOption &other);
    TOption operator|(const TOption &other) const;
};
#endif

typedef QHash<int, QVariant> TOption;

TOption T_CORE_EXPORT operator|(const TOption &o1, const TOption &o2);


#endif // TOPTION_H
