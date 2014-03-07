#ifndef TROUTE_H
#define TROUTE_H

#include <QByteArray>
#include <QString>

class TRoute {
public:
    enum {
        Match = 0,
        Get,
        Post,
        Patch,
        Put,
        Delete,

        Invalid = 0xff
    };

    int     method;
    QString path;
    QByteArray controller;
    QByteArray action;
    bool    params;

    static int methodFromString(QString name);
    QString methodToString(int method);
};

#endif