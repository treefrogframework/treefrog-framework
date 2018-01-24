/* Copyright (c) 2013-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QSettings>
#include <QVariant>
#include <TMongoDriver>
#include <TMongoCursor>
#include "mongocommand.h"

static QSettings *mongoSettings = nullptr;


MongoCommand::MongoCommand(const QString &path)
    : driver(new TMongoDriver), settingsPath(path)
{
    if (!mongoSettings) {
        if (!QFile::exists(settingsPath)) {
            qCritical("not found, %s", qPrintable(path));
        }
        mongoSettings = new QSettings(settingsPath, QSettings::IniFormat);
    }
}


MongoCommand::~MongoCommand()
{
    if (driver->isOpen()) {
        close();
    }
    delete driver;
}


bool MongoCommand::open(const QString &env)
{
    mongoSettings->beginGroup(env);

    databaseName = mongoSettings->value("DatabaseName").toString().trimmed();
    printf("DatabaseName: %s\n", qPrintable(databaseName));

    QString host = mongoSettings->value("HostName").toString().trimmed();
    printf("HostName:     %s\n", qPrintable(host));

    int port = mongoSettings->value("Port").toInt();
    QString user = mongoSettings->value("UserName").toString().trimmed();
    QString pass = mongoSettings->value("Password").toString().trimmed();
    QString opts = mongoSettings->value("ConnectOptions").toString().trimmed();
    mongoSettings->endGroup();

    bool status = driver->open(databaseName, user, pass, host, port, opts);
    if (!status) {
        fprintf(stderr, "MongoDB open error\n");
    } else {
        printf("MongoDB opened successfully\n");
    }
    return status;
}


void MongoCommand::close()
{
    driver->close();
}


QStringList MongoCommand::getCollectionNames() const
{
    QStringList ret;
    if (!driver->isOpen())
        return ret;

    int cnt = driver->find("system.namespaces", QVariantMap(), QVariantMap(), QStringList(), 0, 0, 0);
    if (cnt > 0) {
        while (driver->cursor().next()) {
            QVariantMap val = driver->cursor().value();
            QString coll = val["name"].toString().mid(databaseName.length() + 1);

            if (!coll.contains('$')) {
                ret.prepend(coll);
            }
        }
    }

    qSort(ret);
    return ret;
}
