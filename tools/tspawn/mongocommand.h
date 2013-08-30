#ifndef MONGOCOMMAND_H
#define MONGOCOMMAND_H

#include <QStringList>

class TMongoDriver;


class MongoCommand
{
public:
    MongoCommand(const QString &path);
    ~MongoCommand();

    bool open(const QString &env);
    void close();
    QStringList getCollectionNames() const;

private:
    TMongoDriver *driver;
    QString settingsPath;
    QString databaseName;
    Q_DISABLE_COPY(MongoCommand)
};

#endif // MONGOCOMMAND_H
