/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "modelgenerator.h"
#include "filewriter.h"
#include "global.h"
#include "mongoobjgenerator.h"
#include "projectfilegenerator.h"
#include "sqlobjgenerator.h"
#include "util.h"
#include <tfnamespace.h>

constexpr auto USER_VIRTUAL_METHOD = "identityKey";
constexpr auto LOCK_REVISION_FIELD = "lockRevision";


constexpr auto MODEL_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                            "#include <QStringList>\n"
                                            "#include <QDateTime>\n"
                                            "#include <QVariant>\n"
                                            "#include <QSharedDataPointer>\n"
                                            "#include <TGlobal>\n"
                                            "#include <TAbstractModel>\n"
                                            "\n"
                                            "class TModelObject;\n"
                                            "class %model%Object;\n"
                                            "%7%"
                                            "\n\n"
                                            "class T_MODEL_EXPORT %model% : public TAbstractModel {\n"
                                            "public:\n"
                                            "    %model%();\n"
                                            "    %model%(const %model% &other);\n"
                                            "    %model%(const %model%Object &object);\n"
                                            "    ~%model%();\n"
                                            "\n"
                                            "%setgetDecl%"
                                            "    %model% &operator=(const %model% &other);\n"
                                            "\n"
                                            "    bool create() override { return TAbstractModel::create(); }\n"
                                            "    bool update() override { return TAbstractModel::update(); }\n"
                                            "%upsertDecl%"
                                            "    bool save()   override { return TAbstractModel::save(); }\n"
                                            "    bool remove() override { return TAbstractModel::remove(); }\n"
                                            "\n"
                                            "    static %model% create(%4%);\n"
                                            "    static %model% create(const QVariantMap &values);\n"
                                            "    static %model% get(%5%);\n"
                                            "%6%"
                                            "    static int count();\n"
                                            "    static QList<%model%> getAll();\n"
                                            "%8%"
                                            "\n"
                                            "private:\n"
                                            "    QSharedDataPointer<%model%Object> d;\n"
                                            "\n"
                                            "    TModelObject *modelData() override;\n"
                                            "    const TModelObject *modelData() const override;\n"
                                            "    friend T_MODEL_EXPORT QDataStream &operator<<(QDataStream &ds, const %model% &model);\n"
                                            "    friend T_MODEL_EXPORT QDataStream &operator>>(QDataStream &ds, %model% &model);\n"
                                            "};\n"
                                            "\n"
                                            "Q_DECLARE_METATYPE(%model%)\n"
                                            "Q_DECLARE_METATYPE(QList<%model%>)\n"
                                            "\n";

constexpr auto MODEL_IMPL_TEMPLATE = "#include <TreeFrogModel>\n"
                                     "#include \"%inc%.h\"\n"
                                     "#include \"%objdir%%inc%object.h\"\n"
                                     "\n\n"
                                     "%model%::%model%() :\n"
                                     "    TAbstractModel(),\n"
                                     "    d(new %model%Object())\n"
                                     "{\n"
                                     "    // set the initial parameters\n"
                                     "}\n"
                                     "\n"
                                     "%model%::%model%(const %model% &other) :\n"
                                     "    TAbstractModel(),\n"
                                     "    d(other.d)\n"
                                     "{ }\n"
                                     "\n"
                                     "%model%::%model%(const %model%Object &object) :\n"
                                     "    TAbstractModel(),\n"
                                     "    d(new %model%Object(object))\n"
                                     "{ }\n"
                                     "\n"
                                     "%model%::~%model%()\n"
                                     "{\n"
                                     "    // If the reference count becomes 0,\n"
                                     "    // the shared data object '%model%Object' is deleted.\n"
                                     "}\n"
                                     "\n"
                                     "%4%"
                                     "%model% &%model%::operator=(const %model% &other)\n"
                                     "{\n"
                                     "    d = other.d;  // increments the reference count of the data\n"
                                     "    return *this;\n"
                                     "}\n"
                                     "\n"
                                     "%upsertImpl%"
                                     "%model% %model%::create(%5%)\n"
                                     "{\n"
                                     "%6%"
                                     "}\n"
                                     "\n"
                                     "%model% %model%::create(const QVariantMap &values)\n"
                                     "{\n"
                                     "    %model% model;\n"
                                     "    model.setProperties(values);\n"
                                     "    if (!model.d->create()) {\n"
                                     "        model.d->clear();\n"
                                     "    }\n"
                                     "    return model;\n"
                                     "}\n"
                                     "\n"
                                     "%model% %model%::get(%7%)\n"
                                     "{\n"
                                     "%8%"
                                     "}\n"
                                     "\n"
                                     "%10%"
                                     "int %model%::count()\n"
                                     "{\n"
                                     "    %13%<%model%Object> mapper;\n"
                                     "    return mapper.findCount();\n"
                                     "}\n"
                                     "\n"
                                     "QList<%model%> %model%::getAll()\n"
                                     "{\n"
                                     "    return tfGetModelListBy%11%Criteria<%model%, %model%Object>(TCriteria());\n"
                                     "}\n"
                                     "\n"
                                     "%12%"
                                     "TModelObject *%model%::modelData()\n"
                                     "{\n"
                                     "    return d.data();\n"
                                     "}\n"
                                     "\n"
                                     "const TModelObject *%model%::modelData() const\n"
                                     "{\n"
                                     "    return d.data();\n"
                                     "}\n"
                                     "\n"
                                     "QDataStream &operator<<(QDataStream &ds, const %model% &model)\n"
                                     "{\n"
                                     "    auto varmap = model.toVariantMap();\n"
                                     "    ds << varmap;\n"
                                     "    return ds;\n"
                                     "}\n"
                                     "\n"
                                     "QDataStream &operator>>(QDataStream &ds, %model% &model)\n"
                                     "{\n"
                                     "    QVariantMap varmap;\n"
                                     "    ds >> varmap;\n"
                                     "    model.setProperties(varmap);\n"
                                     "    return ds;\n"
                                     "}\n"
#if QT_VERSION < 0x060000
                                     "\n"
                                     "// Don't remove below this line\n"
                                     "T_REGISTER_STREAM_OPERATORS(%model%)\n";
#else
                                     "";
#endif

constexpr auto USER_MODEL_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                                 "#include <QStringList>\n"
                                                 "#include <QDateTime>\n"
                                                 "#include <QVariant>\n"
                                                 "#include <QSharedDataPointer>\n"
                                                 "#include <TGlobal>\n"
                                                 "#include <TAbstractUser>\n"
                                                 "#include <TAbstractModel>\n"
                                                 "\n"
                                                 "class TModelObject;\n"
                                                 "class %model%Object;\n"
                                                 "%7%"
                                                 "\n\n"
                                                 "class T_MODEL_EXPORT %model% : public TAbstractUser, public TAbstractModel {\n"
                                                 "public:\n"
                                                 "    %model%();\n"
                                                 "    %model%(const %model% &other);\n"
                                                 "    %model%(const %model%Object &object);\n"
                                                 "    ~%model%();\n"
                                                 "\n"
                                                 "%setgetDecl%"
                                                 "%11%"
                                                 "    %model% &operator=(const %model% &other);\n"
                                                 "\n"
                                                 "    bool create() { return TAbstractModel::create(); }\n"
                                                 "    bool update() { return TAbstractModel::update(); }\n"
                                                 "%upsertDecl%"
                                                 "    bool save()   { return TAbstractModel::save(); }\n"
                                                 "    bool remove() { return TAbstractModel::remove(); }\n"
                                                 "\n"
                                                 "    static %model% authenticate(const QString &%9%, const QString &%10%);\n"
                                                 "    static %model% create(%4%);\n"
                                                 "    static %model% create(const QVariantMap &values);\n"
                                                 "    static %model% get(%5%);\n"
                                                 "%6%"
                                                 "    static int count();\n"
                                                 "    static QList<%model%> getAll();\n"
                                                 "%8%"
                                                 "\n"
                                                 "private:\n"
                                                 "    QSharedDataPointer<%model%Object> d;\n"
                                                 "\n"
                                                 "    TModelObject *modelData();\n"
                                                 "    const TModelObject *modelData() const;\n"
                                                 "    friend QDataStream &operator<<(QDataStream &ds, const %model% &model);\n"
                                                 "    friend QDataStream &operator>>(QDataStream &ds, %model% &model);\n"
                                                 "};\n"
                                                 "\n"
                                                 "Q_DECLARE_METATYPE(%model%)\n"
                                                 "Q_DECLARE_METATYPE(QList<%model%>)\n"
                                                 "\n";

constexpr auto USER_MODEL_IMPL_TEMPLATE = "#include <TreeFrogModel>\n"
                                          "#include \"%inc%.h\"\n"
                                          "#include \"%objdir%%inc%object.h\"\n"
                                          "\n\n"
                                          "%model%::%model%() :\n"
                                          "    TAbstractUser(),\n"
                                          "    TAbstractModel(),\n"
                                          "    d(new %model%Object())\n"
                                          "{\n"
                                          "    // set the initial parameters\n"
                                          "}\n"
                                          "\n"
                                          "%model%::%model%(const %model% &other) :\n"
                                          "    TAbstractUser(),\n"
                                          "    TAbstractModel(),\n"
                                          "    d(other.d)\n"
                                          "{ }\n"
                                          "\n"
                                          "%model%::%model%(const %model%Object &object) :\n"
                                          "    TAbstractUser(),\n"
                                          "    TAbstractModel(),\n"
                                          "    d(new %model%Object(object))\n"
                                          "{ }\n"
                                          "\n"
                                          "\n"
                                          "%model%::~%model%()\n"
                                          "{\n"
                                          "    // If the reference count becomes 0,\n"
                                          "    // the shared data object '%model%Object' is deleted.\n"
                                          "}\n"
                                          "\n"
                                          "%4%"
                                          "%model% &%model%::operator=(const %model% &other)\n"
                                          "{\n"
                                          "    d = other.d;  // increments the reference count of the data\n"
                                          "    return *this;\n"
                                          "}\n"
                                          "\n"
                                          "%upsertImpl%"
                                          "%model% %model%::authenticate(const QString &%14%, const QString &%15%)\n"
                                          "{\n"
                                          "    if (%14%.isEmpty() || %15%.isEmpty())\n"
                                          "        return %model%();\n"
                                          "\n"
                                          "    %13%<%model%Object> mapper;\n"
                                          "    %model%Object obj = mapper.findFirst(TCriteria(%model%Object::%16%, %14%));\n"
                                          "    if (obj.isNull() || obj.%17% != %15%) {\n"
                                          "        obj.clear();\n"
                                          "    }\n"
                                          "    return %model%(obj);\n"
                                          "}\n"
                                          "\n"
                                          "%model% %model%::create(%5%)\n"
                                          "{\n"
                                          "%6%"
                                          "}\n"
                                          "\n"
                                          "%model% %model%::create(const QVariantMap &values)\n"
                                          "{\n"
                                          "    %model% model;\n"
                                          "    model.setProperties(values);\n"
                                          "    if (!model.d->create()) {\n"
                                          "        model.d->clear();\n"
                                          "    }\n"
                                          "    return model;\n"
                                          "}\n"
                                          "\n"
                                          "%model% %model%::get(%7%)\n"
                                          "{\n"
                                          "%8%"
                                          "}\n"
                                          "\n"
                                          "%10%"
                                          "int %model%::count()\n"
                                          "{\n"
                                          "    %13%<%model%Object> mapper;\n"
                                          "    return mapper.findCount();\n"
                                          "}\n"
                                          "\n"
                                          "QList<%model%> %model%::getAll()\n"
                                          "{\n"
                                          "    return tfGetModelListBy%11%Criteria<%model%, %model%Object>();\n"
                                          "}\n"
                                          "\n"
                                          "%12%"
                                          "TModelObject *%model%::modelData()\n"
                                          "{\n"
                                          "    return d.data();\n"
                                          "}\n"
                                          "\n"
                                          "const TModelObject *%model%::modelData() const\n"
                                          "{\n"
                                          "    return d.data();\n"
                                          "}\n"
                                          "\n"
                                          "QDataStream &operator<<(QDataStream &ds, const %model% &model)\n"
                                          "{\n"
                                          "    auto varmap = model.toVariantMap();\n"
                                          "    ds << varmap;\n"
                                          "    return ds;\n"
                                          "}\n"
                                          "\n"
                                          "QDataStream &operator>>(QDataStream &ds, %model% &model)\n"
                                          "{\n"
                                          "    QVariantMap varmap;\n"
                                          "    ds >> varmap;\n"
                                          "    model.setProperties(varmap);\n"
                                          "    return ds;\n"
                                          "}\n"
#if QT_VERSION < 0x060000
                                          "\n"
                                          "// Don't remove below this line\n"
                                          "T_REGISTER_STREAM_OPERATORS(%model%)";
#else
                                          "";
#endif

constexpr auto MODEL_IMPL_GETALLJSON = "QJsonArray %model%::getAllJson(const QStringList &properties)\n"
                                       "{\n"
                                       "    return tfConvertToJsonArray(getAll(), properties);\n"
                                       "}\n"
                                       "\n";

constexpr auto MODEL_IMPL_GETALLJSON_MONGO = "QJsonArray %model%::getAllJson(const QStringList &properties)\n"
                                             "{\n"
                                             "    return tfConvertToJsonArray(getAll(), properties);\n"
                                             "}\n"
                                             "\n";


namespace {
const QStringList excludedSetter = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    LOCK_REVISION_FIELD,
};
}


inline QPair<QString, QString> pair(const char *first, const QString &second)
{
    return qMakePair(QString::fromUtf8(first), second);
}


ModelGenerator::ModelGenerator(ModelGenerator::ObjectType type, const QString &model, const QString &table, const QStringList &userModelFields) :
    objectType(type),
    tableName(table),
    userFields(userModelFields)
{
    modelName = (!model.isEmpty()) ? fieldNameToEnumName(model) : fieldNameToEnumName(table);
    switch (type) {
    case Sql:
        objGen = new SqlObjGenerator(model, table);
        break;
    case Mongo:
        objGen = new MongoObjGenerator(model);
        break;
    }
}


ModelGenerator::~ModelGenerator()
{
    delete objGen;
}


bool ModelGenerator::generate(const QString &dstDir, bool userModel)
{
    QStringList files;

    // Generates model object class
    QString obj = objGen->generate(dstDir);
    if (obj.isEmpty()) {
        return false;
    }
    files << obj;

    // Generates user-model
    if (userModel) {
        if (userFields.count() == 2) {
            files << genUserModel(dstDir, userFields.value(0), userFields.value(1));
        } else if (userFields.isEmpty()) {
            files << genUserModel(dstDir);
        } else {
            qCritical("invalid parameters");
            return false;
        }
    } else {
        files << genModel(dstDir);
    }

    // Generates a project file
    ProjectFileGenerator progen(QDir(dstDir).filePath("models.pro"));
    bool ret = progen.add(files);

#ifdef Q_OS_WIN
    if (ret) {
        // Deletes dummy models
        QStringList dummy = {"objects/_dummymodel.h", "objects/_dummymodel.cpp"};
        bool rmd = false;
        for (auto &f : dummy) {
            rmd |= ::remove(QDir(dstDir).filePath(f));
        }
        if (rmd) {
            progen.remove(dummy);
        }
    }
#endif

    return ret;
}


QStringList ModelGenerator::genModel(const QString &dstDir)
{
    QStringList ret;
    QDir dir(dstDir + "/objects");
    auto p = createModelParams();

    QString fileName = dir.filePath(modelName.toLower() + ".h");
    gen(fileName, MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QLatin1String("objects/") + QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    gen(fileName, MODEL_IMPL_TEMPLATE, p.second);
    ret << QLatin1String("objects/") + QFileInfo(fileName).fileName();
    return ret;
}


QStringList ModelGenerator::genUserModel(const QString &dstDir, const QString &usernameField, const QString &passwordField)
{
    QStringList ret;
    QDir dir(dstDir + "/objects");
    auto p = createModelParams();
    QString fileName = dir.filePath(modelName.toLower() + ".h");
    QString userVar = fieldNameToVariableName(usernameField);
    p.first << pair("9", userVar)
            << pair("10", fieldNameToVariableName(passwordField))
            << pair("11", QLatin1String("    QString ") + USER_VIRTUAL_METHOD + "() const { return " + userVar + "(); }\n");

    gen(fileName, USER_MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QLatin1String("objects/") + QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    p.second << pair("14", fieldNameToVariableName(usernameField))
             << pair("15", fieldNameToVariableName(passwordField))
             << pair("16", fieldNameToEnumName(usernameField))
             << pair("17", passwordField);
    gen(fileName, USER_MODEL_IMPL_TEMPLATE, p.second);
    ret << QLatin1String("objects/") + QFileInfo(fileName).fileName();
    return ret;
}


QPair<PlaceholderList, PlaceholderList> ModelGenerator::createModelParams()
{
    QString setgetDecl, setgetImpl, crtparams, getOptDecl, getOptImpl;
    QList<QPair<QString, QString>> writableFields;
    bool optlockMethod = false;
    FieldList fields = objGen->fieldList();
    int pkidx = objGen->primaryKeyIndex();
    int autoIndex = objGen->autoValueIndex();
    QString autoFieldName = (autoIndex >= 0) ? fields[autoIndex].first : QString();
    QString mapperstr = (objectType == Sql) ? "TSqlORMapper" : "TMongoODMapper";

    for (QListIterator<QPair<QString, QMetaType::Type>> it(fields); it.hasNext();) {
        const QPair<QString, QMetaType::Type> &p = it.next();
        QString var = fieldNameToVariableName(p.first);
#if QT_VERSION < 0x060000
        QString type = QString::fromLatin1(QVariant::typeToName(p.second));
#else
        QString type = QString::fromLatin1(QMetaType(p.second).name());
#endif
        if (type.isEmpty())
            continue;

        // Getter method
        setgetDecl += QString("    %1 %2() const;\n").arg(type, var);
        setgetImpl += QString("%1 %2::%3() const\n{\n    return d->%4;\n}\n\n").arg(type, modelName, var, p.first);

        if (!excludedSetter.contains(p.first, Qt::CaseInsensitive) && p.first != autoFieldName) {
            // Setter method
            QString str = fieldNameToEnumName(p.first);
            setgetDecl += QString("    void set%1(%2);\n").arg(str, createParam(p.second, p.first));
            setgetImpl += QString("void %1::set%2(%3)\n{\n    d->%4 = %5;\n}\n\n").arg(modelName, str, createParam(p.second, p.first), p.first, var);

            // Appends to crtparams-string
            crtparams += createParam(p.second, p.first);
            crtparams += ", ";

            writableFields << QPair<QString, QString>(p.first, type);
        }

        if (var == LOCK_REVISION_FIELD) {
            optlockMethod = true;
        }
    }
    crtparams.chop(2);

    if (crtparams.isEmpty()) {
        crtparams += "const QString &";
    }

    // Creates parameters of get() method
    QString getparams;
    if (pkidx < 0) {
        getparams = crtparams;
    } else {
        const QPair<QString, QMetaType::Type> &pair = fields[pkidx];
        getparams = createParam(pair.second, pair.first);
    }

    // Creates a declaration and a implementation of 'get' method for optimistic lock
    if (pkidx >= 0 && optlockMethod) {
        const QPair<QString, QMetaType::Type> &pair = fields[pkidx];
        getOptDecl = QString("    static %1 get(%2, int lockRevision);\n").arg(modelName, createParam(pair.second, pair.first));

        getOptImpl = QString("%1 %1::get(%2, int lockRevision)\n"
                             "{\n"
                             "    %5<%1Object> mapper;\n"
                             "    TCriteria cri;\n"
                             "    cri.add(%1Object::%3, %4);\n"
                             "    cri.add(%1Object::LockRevision, lockRevision);\n"
                             "    return %1(mapper.findFirst(cri));\n"
                             "}\n\n")
                         .arg(modelName, createParam(pair.second, pair.first), fieldNameToEnumName(pair.first), fieldNameToVariableName(pair.first), mapperstr);
    }

    PlaceholderList headerList, implList;

    headerList << pair("model", modelName)
               << pair("setgetDecl", setgetDecl)
               << pair("4", crtparams)
               << pair("5", getparams)
               << pair("6", getOptDecl);

    QString upsertDecl, upsertImpl;
    if (objectType == Mongo) {
        upsertDecl = "    bool upsert(const QVariantMap &criteria);\n";
        upsertImpl = QString("bool %1::upsert(const QVariantMap &criteria)\n"
                             "{\n"
                             "    auto *obj = dynamic_cast<TMongoObject*>(modelData());\n"
                             "    return (obj) ? obj->upsert(criteria) : false;\n"
                             "}\n\n")
                         .arg(modelName);
    }
    headerList << pair("upsertDecl", upsertDecl);
    implList << pair("upsertImpl", upsertImpl);

    // Creates a model implementation
    QString createImpl;
    createImpl += QString("    %1Object obj;\n").arg(modelName);

    QListIterator<QPair<QString, QString>> fi(writableFields);
    while (fi.hasNext()) {
        const QPair<QString, QString> &p = fi.next();
        createImpl += QString("    obj.%1 = %2;\n").arg(p.first, fieldNameToVariableName(p.first));
    }
    createImpl += "    if (!obj.create()) {\n";
    createImpl += QString("        return %1();\n").arg(modelName);
    createImpl += "    }\n";
    createImpl += QString("    return %1(obj);\n").arg(modelName);

    // Creates a implementation of get() method
    QString getImpl;
    if (pkidx < 0) {
        // If no primary index exists
        getImpl += QString("    TCriteria cri;\n");
        fi.toFront();
        while (fi.hasNext()) {
            const QPair<QString, QString> &p = fi.next();
            getImpl += QString("    cri.add(%1Object::%2, %3);\n").arg(modelName, fieldNameToEnumName(p.first), fieldNameToVariableName(p.first));
        }
    }

    getImpl += QString("    %1<%2Object> mapper;\n").arg(mapperstr, modelName);
    getImpl += QString("    return %1(mapper.").arg(modelName);

    if (pkidx < 0) {
        getImpl += "findFirst(cri));\n";
    } else {
        const QPair<QString, QMetaType::Type> &pair = fields[pkidx];
        getImpl += (objectType == Sql) ? "findByPrimaryKey(" : "findByObjectId(";
        getImpl += fieldNameToVariableName(pair.first);
        getImpl += QString("));\n");
    }

    implList << pair("inc", modelName.toLower())
             << pair("model", modelName)
             << pair("4", setgetImpl)
             << pair("5", crtparams)
             << pair("6", createImpl)
             << pair("7", getparams)
             << pair("8", getImpl)
             << pair("10", getOptImpl)
             << pair("11", ((objectType == Mongo) ? "Mongo" : ""));

    headerList << pair("7", "class QJsonArray;\n")
               << pair("8", "    static QJsonArray getAllJson(const QStringList &properties = QStringList());\n");

    switch (objectType) {
    case Sql:
        implList << pair("12", replaceholder(MODEL_IMPL_GETALLJSON, pair("model", modelName)));
        implList << pair("objdir", "sqlobjects/");
        break;

    case Mongo:
        implList << pair("12", replaceholder(MODEL_IMPL_GETALLJSON_MONGO, pair("model", modelName)));
        implList << pair("objdir", "mongoobjects/");
        break;

    default:
        implList << pair("12", "");
        implList << pair("objdir", "");
        break;
    }

    implList << pair("13", mapperstr);
    return qMakePair(headerList, implList);
}


void ModelGenerator::gen(const QString &fileName, const QString &format, const QList<QPair<QString, QString>> &values)
{
    QString out = replaceholder(format, values);
    FileWriter fw(fileName);
    fw.write(out, false);
}


QString ModelGenerator::createParam(QMetaType::Type type, const QString &name)
{
    QString string;
    QString var = fieldNameToVariableName(name);

    switch (type) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
#if QT_VERSION < 0x060000
        string += QVariant::typeToName(type);
#else
        string += QString::fromLatin1(QMetaType(type).name());
#endif
        string += ' ';
        string += var;
        break;

    default:
#if QT_VERSION < 0x060000
        string += QString("const %1 &%2").arg(QVariant::typeToName(type), var);
#else
        string += QString("const %1 &%2").arg(QMetaType(type).name(), var);
#endif
        break;
    }
    return string;
}


ModelGenerator::FieldList ModelGenerator::fieldList() const
{
    return objGen->fieldList();
}


int ModelGenerator::primaryKeyIndex() const
{
    return objGen->primaryKeyIndex();
}


int ModelGenerator::autoValueIndex() const
{
    return objGen->autoValueIndex();
}


int ModelGenerator::lockRevisionIndex() const
{
    return objGen->lockRevisionIndex();
}
