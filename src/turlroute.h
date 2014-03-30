#ifndef TURLROUTE_H
#define TURLROUTE_H

#include <QByteArray>
#include <QStringList>
#include <TGlobal>
#include "troute.h"

class TRouting {
public:
    bool empty;
    QString controller;
    QString action;
    QStringList params;

    TRouting();
    TRouting(const QString &controller, const QString &action, const QStringList &params = QStringList());
    bool isEmpty() const { return empty; }
    bool isAllowed() const { return !empty && !controller.isEmpty(); }

    QString toString() { return QString("-> %1#%2 params: [%3]").arg(QString(controller)).arg(QString(action)).arg(params.join(", ")); }
};


inline TRouting::TRouting()
    : empty(true) { }


inline TRouting::TRouting(const QString &ctrl, const QString &act, const QStringList &p)
    : empty(false), controller(ctrl), action(act), params(p) { }


class T_CORE_EXPORT TUrlRoute
{
public:
    static void instantiate();
    static const TUrlRoute &instance();
    TRouting findRouting(Tf::HttpMethod method, const QString &path) const;

    bool addRouteFromString(QString line);
    TUrlRoute() { }
private:
    bool parseConfigFile();

    QList<TRoute> routes;
};

#endif // TURLROUTE_H
