#pragma once
#include "abstractobjgenerator.h"
#include "global.h"
#include <QDir>
#include <QPair>
#include <QStringList>
#include <QVariant>

class AbstractObjGenerator;


class ModelGenerator {
public:
    using FieldList = QList<QPair<QString, QMetaType::Type>>;

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
    static QString createParam(QMetaType::Type type, const QString &name);

protected:
    QStringList genModel(const QString &dstDir);
    QStringList genUserModel(const QString &dstDir, const QString &usernameField = "username", const QString &passwordField = "password");
    QPair<PlaceholderList, PlaceholderList> createModelParams();

    static void gen(const QString &fileName, const QString &format, const QList<QPair<QString, QString>> &values);

private:
    ObjectType objectType {Sql};
    QString modelName;
    QString tableName;
    AbstractObjGenerator *objGen = nullptr;
    QStringList userFields;
};
