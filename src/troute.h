#ifndef TROUTE_H
#define TROUTE_H

#include <QByteArray>
#include <QStringList>

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
    QStringList components;
    QByteArray controller;
    QByteArray action;
    bool has_variable_params;

    static int methodFromString(QString name);
    QString methodToString(int method);
};

#endif