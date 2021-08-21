/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "mongocommand.h"
#include <QFile>
#include <QSettings>
#include <QVariant>
#include <TMongoCursor>
#include <TMongoDriver>

static QSettings *mongoSettings = nullptr;


MongoCommand::MongoCommand(const QString &path) :
    driver(new TMongoDriver),
    settingsPath(path)
{
    if (!mongoSettings) {
        if (!QFile::exists(settingsPath)) {
            qCritical("not found, %s", qUtf8Printable(path));
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
    databaseName = mongoSettings->value(env + "/DatabaseName").toString().trimmed();
    std::printf("DatabaseName: %s\n", qUtf8Printable(databaseName));

    QString host = mongoSettings->value(env + "/HostName").toString().trimmed();
    std::printf("HostName:     %s\n", qUtf8Printable(host));

    int port = mongoSettings->value(env + "/Port").toInt();
    QString user = mongoSettings->value(env + "/UserName").toString().trimmed();
    QString pass = mongoSettings->value(env + "/Password").toString().trimmed();
    QString opts = mongoSettings->value(env + "/ConnectOptions").toString().trimmed();

    bool status = driver->open(databaseName, user, pass, host, port, opts);
    if (!status) {
        std::fprintf(stderr, "MongoDB open error\n");
    } else {
        std::printf("MongoDB opened successfully\n");
    }
    return status;
}


void MongoCommand::close()
{
    driver->close();
}


QStringList MongoCommand::getCollectionNames() const
{
    return (driver->isOpen()) ? driver->getCollectionNames() : QStringList();
}
