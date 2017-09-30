#ifndef MODELGENERATOR_H
#define MODELGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>

class AbstractObjGenerator;


class ModelGenerator
{
public:
    typedef QList<QPair<QString, QVariant::Type>>  FieldList;
    typedef QList<QPair<QString, QString>>  PlaceholderList;

    enum ObjectType {
        Sql,
        Mongo,
    };

    ModelGenerator(ObjectType type, const QString &model, const QString &table = QString(), const QStringList &userModelFields = QStringList());
    ~ModelGenerator();

    bool generate(const QString &dst, bool userModel = false);
    FieldList fieldList() const;
    int primaryKeyIndex() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;
    QString model() const { return modelName; }
    static QString replaceholder(const QString &format, const QPair<QString, QString> &value);
    static QString replaceholder(const QString &format, const PlaceholderList &values);

protected:
    QStringList genModel(const QString &dstDir);
    QStringList genUserModel(const QString &dstDir, const QString &usernameField = "username", const QString &passwordField = "password");
    QPair<PlaceholderList, PlaceholderList> createModelParams();

    static void gen(const QString &fileName, const QString &format, const QList<QPair<QString, QString>> &values);
    static QString createParam(QVariant::Type type, const QString &name);

private:
    ObjectType objectType;
    QString modelName;
    QString tableName;
    AbstractObjGenerator *objGen;
    QStringList userFields;
};

#endif // MODELGENERATOR_H
