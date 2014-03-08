#ifndef TURLROUTE_H
#define TURLROUTE_H

#include <QByteArray>
#include <QStringList>
#include <TGlobal>
#include "troute.h"

class TRouting {
public:
    bool empty;
    QByteArray controller;
    QByteArray action;
    QStringList params;

    TRouting();
    TRouting(const QByteArray &controller, const QByteArray &action, const QStringList &params = QStringList());
    bool isEmpty() const { return empty; }
    bool isAllowed() const { return !empty && !controller.isEmpty(); }
};


inline TRouting::TRouting()
    : empty(true) { }


inline TRouting::TRouting(const QByteArray &ctrl, const QByteArray &act, const QStringList &p)
    : empty(false), controller(ctrl), action(act), params(p) { }


class T_CORE_EXPORT TUrlRoute
{
public:
    static void instantiate();
    static const TUrlRoute &instance();
    TRouting findRouting(Tf::HttpMethod method, const QString &path) const;

private:
    TUrlRoute() { }
    bool parseConfigFile();

    QList<TRoute> routes;
};

#endif // TURLROUTE_H
