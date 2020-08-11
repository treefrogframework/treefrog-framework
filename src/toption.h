#pragma once
#include <QMap>
#include <QVariant>
#include <TGlobal>

using TOption = QMap<int, QVariant>;

TOption T_CORE_EXPORT operator|(const TOption &o1, const TOption &o2);

