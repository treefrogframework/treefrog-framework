/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "modelgenerator.h"
#include "sqlobjgenerator.h"
#include "mongoobjgenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "util.h"

#define USER_VIRTUAL_METHOD  "identityKey"
#define LOCK_REVISION_FIELD "lockRevision"

#define MODEL_HEADER_FILE_TEMPLATE                       \
    "#ifndef %1_H\n"                                     \
    "#define %1_H\n"                                     \
    "\n"                                                 \
    "#include <QStringList>\n"                           \
    "#include <QDateTime>\n"                             \
    "#include <QVariant>\n"                              \
    "#include <QSharedDataPointer>\n"                    \
    "#include <TGlobal>\n"                               \
    "#include <TAbstractModel>\n"                        \
    "\n"                                                 \
    "class TModelObject;\n"                              \
    "class %2Object;\n"                                  \
    "%7"                                                 \
    "\n\n"                                               \
    "class T_MODEL_EXPORT %2 : public TAbstractModel\n"  \
    "{\n"                                                \
    "public:\n"                                          \
    "    %2();\n"                                        \
    "    %2(const %2 &other);\n"                         \
    "    %2(const %2Object &object);\n"                  \
    "    ~%2();\n"                                       \
    "\n"                                                 \
    "%3"                                                 \
    "    %2 &operator=(const %2 &other);\n"              \
    "\n"                                                 \
    "    bool create() { return TAbstractModel::create(); }\n" \
    "    bool update() { return TAbstractModel::update(); }\n" \
    "    bool save()   { return TAbstractModel::save(); }\n"   \
    "    bool remove() { return TAbstractModel::remove(); }\n" \
    "\n"                                                 \
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "%6"                                                 \
    "    static int count();\n"                          \
    "    static QList<%2> getAll();\n"                   \
    "%8"                                                 \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TModelObject *modelData();\n"                   \
    "    const TModelObject *modelData() const;\n"       \
    "};\n"                                               \
    "\n"                                                 \
    "Q_DECLARE_METATYPE(%2)\n"                           \
    "Q_DECLARE_METATYPE(QList<%2>)\n"                    \
    "\n"                                                 \
    "#endif // %1_H\n"

#define MODEL_IMPL_TEMPLATE                                   \
    "#include <TreeFrogModel>\n"                              \
    "#include \"%1.h\"\n"                                     \
    "#include \"%1object.h\"\n"                               \
    "\n"                                                      \
    "%2::%2()\n"                                              \
    "    : TAbstractModel(), d(new %2Object)\n"               \
    "{%3}\n"                                                  \
    "\n"                                                      \
    "%2::%2(const %2 &other)\n"                               \
    "    : TAbstractModel(), d(new %2Object(*other.d))\n"     \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::%2(const %2Object &object)\n"                        \
    "    : TAbstractModel(), d(new %2Object(object))\n"       \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::~%2()\n"                                             \
    "{\n"                                                     \
    "    // If the reference count becomes 0,\n"              \
    "    // the shared data object '%2Object' is deleted.\n"  \
    "}\n"                                                     \
    "\n"                                                      \
    "%4"                                                      \
    "%2 &%2::operator=(const %2 &other)\n"                    \
    "{\n"                                                     \
    "    d = other.d;  // increments the reference count of the data\n" \
    "    return *this;\n"                                     \
    "}\n\n"                                                   \
    "%2 %2::create(%5)\n"                                     \
    "{\n"                                                     \
    "%6"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(const QVariantMap &values)\n"              \
    "{\n"                                                     \
    "    %2 model;\n"                                         \
    "    model.setProperties(values);\n"                      \
    "    if (!model.d->create()) {\n"                         \
    "        model.d->clear();\n"                             \
    "    }\n"                                                 \
    "    return model;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::get(%7)\n"                                        \
    "{\n"                                                     \
    "%8"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%10"                                                     \
    "int %2::count()\n"                                       \
    "{\n"                                                     \
    "    %13<%2Object> mapper;\n"                             \
    "    return mapper.findCount();\n"                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListBy%11Criteria<%2, %2Object>(TCriteria());\n" \
    "}\n"                                                     \
    "\n"                                                      \
    "%12"                                                     \
    "TModelObject *%2::modelData()\n"                         \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TModelObject *%2::modelData() const\n"             \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"

#define USER_MODEL_HEADER_FILE_TEMPLATE                  \
    "#ifndef %1_H\n"                                     \
    "#define %1_H\n"                                     \
    "\n"                                                 \
    "#include <QStringList>\n"                           \
    "#include <QDateTime>\n"                             \
    "#include <QVariant>\n"                              \
    "#include <QSharedDataPointer>\n"                    \
    "#include <TGlobal>\n"                               \
    "#include <TAbstractUser>\n"                         \
    "#include <TAbstractModel>\n"                        \
    "\n"                                                 \
    "class TModelObject;\n"                              \
    "class %2Object;\n"                                  \
    "%7"                                                 \
    "\n\n"                                               \
    "class T_MODEL_EXPORT %2 : public TAbstractUser, public TAbstractModel\n" \
    "{\n"                                                \
    "public:\n"                                          \
    "    %2();\n"                                        \
    "    %2(const %2 &other);\n"                         \
    "    %2(const %2Object &object);\n"                  \
    "    ~%2();\n"                                       \
    "\n"                                                 \
    "%3"                                                 \
    "%11"                                                \
    "    %2 &operator=(const %2 &other);\n"              \
    "\n"                                                 \
    "    bool create() { return TAbstractModel::create(); }\n" \
    "    bool update() { return TAbstractModel::update(); }\n" \
    "    bool save()   { return TAbstractModel::save(); }\n"   \
    "    bool remove() { return TAbstractModel::remove(); }\n" \
    "\n"                                                 \
    "    static %2 authenticate(const QString &%9, const QString &%10);\n" \
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "%6"                                                 \
    "    static int count();\n"                          \
    "    static QList<%2> getAll();\n"                   \
    "%8"                                                 \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TModelObject *modelData();\n"                   \
    "    const TModelObject *modelData() const;\n"       \
    "};\n"                                               \
    "\n"                                                 \
    "Q_DECLARE_METATYPE(%2)\n"                           \
    "Q_DECLARE_METATYPE(QList<%2>)\n"                    \
    "\n"                                                 \
    "#endif // %1_H\n"

#define USER_MODEL_IMPL_TEMPLATE                              \
    "#include <TreeFrogModel>\n"                              \
    "#include \"%1.h\"\n"                                     \
    "#include \"%1object.h\"\n"                               \
    "\n"                                                      \
    "%2::%2()\n"                                              \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object)\n" \
    "{%3}\n"                                                  \
    "\n"                                                      \
    "%2::%2(const %2 &other)\n"                               \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object(*other.d))\n" \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::%2(const %2Object &object)\n"                        \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object(object))\n" \
    "{ }\n"                                                   \
    "\n"                                                      \
    "\n"                                                      \
    "%2::~%2()\n"                                             \
    "{\n"                                                     \
    "    // If the reference count becomes 0,\n"              \
    "    // the shared data object '%2Object' is deleted.\n"  \
    "}\n"                                                     \
    "\n"                                                      \
    "%4"                                                      \
    "%2 &%2::operator=(const %2 &other)\n"                    \
    "{\n"                                                     \
    "    d = other.d;  // increments the reference count of the data\n" \
    "    return *this;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::authenticate(const QString &%14, const QString &%15)\n" \
    "{\n"                                                     \
    "    if (%14.isEmpty() || %15.isEmpty())\n"               \
    "        return %2();\n"                                  \
    "\n"                                                      \
    "    %13<%2Object> mapper;\n"                             \
    "    %2Object obj = mapper.findFirst(TCriteria(%2Object::%16, %14));\n" \
    "    if (obj.isNull() || obj.%17 != %15) {\n"             \
    "        obj.clear();\n"                                  \
    "    }\n"                                                 \
    "    return %2(obj);\n"                                   \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(%5)\n"                                     \
    "{\n"                                                     \
    "%6"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(const QVariantMap &values)\n"              \
    "{\n"                                                     \
    "    %2 model;\n"                                         \
    "    model.setProperties(values);\n"                      \
    "    if (!model.d->create()) {\n"                         \
    "        model.d->clear();\n"                             \
    "    }\n"                                                 \
    "    return model;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::get(%7)\n"                                        \
    "{\n"                                                     \
    "%8"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%10"                                                     \
    "int %2::count()\n"                                       \
    "{\n"                                                     \
    "    %13<%2Object> mapper;\n"                             \
    "    return mapper.findCount();\n"                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListBy%11Criteria<%2, %2Object>();\n" \
    "}\n"                                                     \
    "\n"                                                      \
    "%12"                                                     \
    "TModelObject *%2::modelData()\n"                         \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TModelObject *%2::modelData() const\n"             \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"

#define MODEL_IMPL_GETALLJSON                                 \
    "QJsonArray %1::getAllJson()\n"                           \
    "{\n"                                                     \
    "    QJsonArray array;\n"                                 \
    "    TSqlORMapper<%1Object> mapper;\n"                    \
    "\n"                                                      \
    "    if (mapper.find() > 0) {\n"                          \
    "        for (TSqlORMapperIterator<%1Object> i(mapper); i.hasNext(); ) {\n" \
    "            array.append(QJsonValue(QJsonObject::fromVariantMap(%1(i.next()).toVariantMap())));\n" \
    "        }\n"                                             \
    "    }\n"                                                 \
    "    return array;\n"                                     \
    "}\n"                                                     \
    "\n"

#define MODEL_IMPL_GETALLJSON_MONGO                           \
    "QJsonArray %1::getAllJson()\n"                           \
    "{\n"                                                     \
    "    QJsonArray array;\n"                                 \
    "    TMongoODMapper<%1Object> mapper;\n"                  \
    "\n"                                                      \
    "    if (mapper.find() > 0) {\n"                          \
    "        while (mapper.next()) {\n"                       \
    "            array.append(QJsonValue(QJsonObject::fromVariantMap(%1(mapper.value()).toVariantMap())));\n" \
    "        }\n"                                             \
    "    }\n"                                                 \
    "    return array;\n"                                     \
    "}\n"                                                     \
    "\n"

static const QStringList excludedSetter = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    LOCK_REVISION_FIELD,
};


ModelGenerator::ModelGenerator(ModelGenerator::ObjectType type, const QString &model, const QString &table, const QStringList &userModelFields)
    : objectType(type), modelName(), tableName(table), userFields(userModelFields)
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
        QStringList dummy = { "_dummymodel.h", "_dummymodel.cpp" };
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
    QDir dir(dstDir);
    QPair<QStringList, QStringList> p = createModelParams();

    QString fileName = dir.filePath(modelName.toLower() + ".h");
    gen(fileName, MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    gen(fileName, MODEL_IMPL_TEMPLATE, p.second);
    ret << QFileInfo(fileName).fileName();
    return ret;
}


QStringList ModelGenerator::genUserModel(const QString &dstDir, const QString &usernameField, const QString &passwordField)
{
    QStringList ret;
    QDir dir(dstDir);
    QPair<QStringList, QStringList> p = createModelParams();
    QString fileName = dir.filePath(modelName.toLower() + ".h");
    QString userVar = fieldNameToVariableName(usernameField);
    p.first << userVar << fieldNameToVariableName(passwordField);
    p.first << QLatin1String("    QString ") + USER_VIRTUAL_METHOD + "() const { return " + userVar + "(); }\n";

    gen(fileName, USER_MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    p.second << fieldNameToVariableName(usernameField) << fieldNameToVariableName(passwordField)
             << fieldNameToEnumName(usernameField) << passwordField;
    gen(fileName, USER_MODEL_IMPL_TEMPLATE, p.second);
    ret << QFileInfo(fileName).fileName();
    return ret;
}


QPair<QStringList, QStringList> ModelGenerator::createModelParams()
{
    QString setgetDecl, setgetImpl, crtparams, getOptDecl, getOptImpl, initParams;
    QList<QPair<QString, QString>> writableFields;
    bool optlockMethod = false;
    FieldList fields = objGen->fieldList();
    int pkidx = objGen->primaryKeyIndex();
    int autoIndex = objGen->autoValueIndex();
    QString autoFieldName = (autoIndex >= 0) ? fields[autoIndex].first : QString();
    QString mapperstr = (objectType == Sql) ? "TSqlORMapper" : "TMongoODMapper";

    for (QListIterator<QPair<QString, QVariant::Type>> it(fields); it.hasNext(); ) {
        const QPair<QString, QVariant::Type> &p = it.next();
        QString var = fieldNameToVariableName(p.first);
        QString type = QVariant::typeToName(p.second);
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

        // Initial value in the default constructor
        switch ((int)p.second) {
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Double:
            initParams += QString("\n    d->") + p.first + " = 0;";
            break;
        }

        if (var == LOCK_REVISION_FIELD)
            optlockMethod = true;
    }
    crtparams.chop(2);

    initParams += (initParams.isEmpty()) ? ' ' : '\n';

    // Creates parameters of get() method
    QString getparams;
    if (pkidx < 0) {
        getparams = crtparams;
    } else {
        const QPair<QString, QVariant::Type> &pair = fields[pkidx];
        getparams = createParam(pair.second, pair.first);
    }

    // Creates a declaration and a implementation of 'get' method for optimistic lock
    if (pkidx >= 0 && optlockMethod) {
        const QPair<QString, QVariant::Type> &pair = fields[pkidx];
        getOptDecl = QString("    static %1 get(%2, int lockRevision);\n").arg(modelName, createParam(pair.second, pair.first));

        getOptImpl = QString("%1 %1::get(%2, int lockRevision)\n"       \
                             "{\n"                                      \
                             "    %5<%1Object> mapper;\n"               \
                             "    TCriteria cri;\n"                     \
                             "    cri.add(%1Object::%3, %4);\n"         \
                             "    cri.add(%1Object::LockRevision, lockRevision);\n" \
                             "    return %1(mapper.findFirst(cri));\n"  \
                             "}\n\n").arg(modelName, createParam(pair.second, pair.first), fieldNameToEnumName(pair.first), fieldNameToVariableName(pair.first), mapperstr);
    }

    QStringList headerArgs;
    headerArgs << modelName.toUpper() << modelName << setgetDecl << crtparams << getparams << getOptDecl;

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
        const QPair<QString, QVariant::Type> &pair = fields[pkidx];
        getImpl += (objectType == Sql) ? "findByPrimaryKey(" : "findByObjectId(";
        getImpl += fieldNameToVariableName(pair.first);
        getImpl += QString("));\n");
    }

    QStringList implArgs;
    implArgs << modelName.toLower() << modelName << initParams << setgetImpl << crtparams << createImpl << getparams << getImpl << getOptImpl;
    implArgs << ((objectType == Mongo) ? "Mongo" : "");

#if QT_VERSION >= 0x050000
    headerArgs  << "class QJsonArray;\n" << "    static QJsonArray getAllJson();\n";
    switch (objectType) {
    case Sql:
        implArgs << QString(MODEL_IMPL_GETALLJSON).arg(modelName);
        break;

    case Mongo:
        implArgs << QString(MODEL_IMPL_GETALLJSON_MONGO).arg(modelName);
        break;

    default:
        implArgs << "";
        break;
    }
#else
    headerArgs << "" << "";
    implArgs << "";
#endif

    implArgs << mapperstr;
    return QPair<QStringList, QStringList>(headerArgs, implArgs);
}


bool ModelGenerator::gen(const QString &fileName, const QString &format, const QStringList &args)
{
    QString out = format;
    for (QStringListIterator i(args); i.hasNext(); ) {
        out = out.arg(i.next());
    }

    FileWriter fw(fileName);
    fw.write(out, false);
    return true;
}


QString ModelGenerator::createParam(QVariant::Type type, const QString &name)
{
    QString string;
    QString var = fieldNameToVariableName(name);
    if (type == QVariant::Int || type == QVariant::UInt || type == QVariant::ULongLong || type == QVariant::Double) {
        string += QVariant::typeToName(type);
        string += ' ';
        string += var;
    } else {
        string += QString("const %1 &%2").arg(QVariant::typeToName(type), var);
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
