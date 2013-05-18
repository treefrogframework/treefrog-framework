/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "modelgenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "tableschema.h"

#define USER_VIRTUAL_METHOD  "identityKey"
#define LOCK_REVISION_FIELD  "lock_revision"

#define SQLOBJECT_HEADER_TEMPLATE                            \
    "#ifndef %1OBJECT_H\n"                                   \
    "#define %1OBJECT_H\n"                                   \
    "\n"                                                     \
    "#include <TSqlObject>\n"                                \
    "#include <QSharedData>\n"                               \
    "\n\n"                                                   \
    "class T_MODEL_EXPORT %2Object : public TSqlObject, public QSharedData\n" \
    "{\n"                                                    \
    "public:\n"

#define SQLOBJECT_PROPERTY_TEMPLATE                  \
    "    Q_PROPERTY(%1 %2 READ get%2 WRITE set%2)\n" \
    "    T_DEFINE_PROPERTY(%1, %2)\n"

#define SQLOBJECT_FOOTER_TEMPLATE  \
    "};\n"                         \
    "\n"                           \
    "#endif // %1OBJECT_H\n"

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
    "class TSqlObject;\n"                                \
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
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "%6"                                                 \
    "    static QList<%2> getAll();\n"                   \
    "%8"                                                 \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TSqlObject *data();\n"                          \
    "    const TSqlObject *data() const;\n"              \
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
    "%9"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListByCriteria<%2, %2Object>(TCriteria());\n" \
    "}\n"                                                     \
    "\n"                                                      \
    "%10"                                                     \
    "TSqlObject *%2::data()\n"                                \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TSqlObject *%2::data() const\n"                    \
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
    "class TSqlObject;\n"                                \
    "class %2Object;\n"                                  \
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
    "%9"                                                 \
    "    %2 &operator=(const %2 &other);\n"              \
    "\n"                                                 \
    "    static %2 authenticate(const QString &%7, const QString &%8);\n" \
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "%6"                                                 \
    "    static QList<%2> getAll();\n"                   \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TSqlObject *data();\n"                          \
    "    const TSqlObject *data() const;\n"              \
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
    "%2 %2::authenticate(const QString &%10, const QString &%11)\n" \
    "{\n"                                                     \
    "    if (%10.isEmpty() || %11.isEmpty())\n"               \
    "        return %2();\n"                                  \
    "\n"                                                      \
    "    TSqlORMapper<%2Object> mapper;\n"                    \
    "    %2Object obj = mapper.findFirst(TCriteria(%2Object::%12, %10));\n" \
    "    if (obj.isNull() || obj.%11 != %11) {\n"             \
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
    "%9"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListByCriteria<%2, %2Object>();\n"  \
    "}\n"                                                     \
    "\n"                                                      \
    "TSqlObject *%2::data()\n"                                \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TSqlObject *%2::data() const\n"                    \
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

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, excludedSetter,
{
    *x << "created_at" << "updated_at" << "modified_at" << LOCK_REVISION_FIELD;
})


ModelGenerator::ModelGenerator(const QString &model, const QString &table, const QStringList &fields, const QString &dst)
    : modelName(), tableName(table), dstDir(dst), fieldList(fields)
{
    modelName = (!model.isEmpty()) ? model : fieldNameToEnumName(table);
}


bool ModelGenerator::generate(bool userModel) const
{
    QStringList files;

    if (!TableSchema(tableName).exists()) {
        qCritical("table not found, %s", qPrintable(tableName));
        return false;
    }

    // Generates models
    files << generateSqlObject();
    if (userModel) {
        if (fieldList.count() == 2) {
            files << genUserModel(fieldList.value(0), fieldList.value(1));
        } else if (fieldList.isEmpty()) {
            files << genUserModel();
        } else {
            qCritical("invalid parameters");
            return false;
        }
    } else {
        files << genModel();
    }

    // Generates a project file
    ProjectFileGenerator progen(dstDir.filePath("models.pro"));
    return progen.add(files);
}


QString ModelGenerator::generateSqlObject() const
{
    TableSchema ts(tableName);
    QList<QPair<QString, QString> > fieldList = ts.getFieldList();
    if (fieldList.isEmpty()) {
        qCritical("table not found, %s", qPrintable(tableName));
        return QString();
    }

    QString output;

    // Header part
    output += QString(SQLOBJECT_HEADER_TEMPLATE).arg(modelName.toUpper(), modelName);
    QListIterator<QPair<QString, QString> > it(fieldList);
    while (it.hasNext()) {
        const QPair<QString, QString> &p = it.next();
        output += QString("    %1 %2;\n").arg(p.second, p.first);
    }

    // enum part
    output += QLatin1String("\n    enum PropertyIndex {\n");
    it.toFront();
    const QPair<QString, QString> &p = it.next();
    output += QString("        %1 = 0,\n").arg(fieldNameToEnumName(p.first));
    while (it.hasNext()) {
        const QPair<QString, QString> &p = it.next();
        output += QString("        %1,\n").arg(fieldNameToEnumName(p.first));
    }
    output += QLatin1String("    };\n\n");

    // primaryKeyIndex() method
    output += QLatin1String("    int primaryKeyIndex() const { return ");
    QString pkName = ts.primaryKeyFieldName();
    if (pkName.isEmpty()) {
        output += QLatin1String("-1; }\n");
    } else {
        output += fieldNameToEnumName(pkName);
        output += QLatin1String("; }\n");
    }

    // auto-value field, for example auto-increment value
    output += QLatin1String("    int autoValueIndex() const { return ");
    QString autoValue = ts.autoValueFieldName();
    if (autoValue.isEmpty()) {
        output += QLatin1String("-1; }\n");
    } else {
        output += fieldNameToEnumName(autoValue);
        output += QLatin1String("; }\n");
    }

    // tableName() method
    output += QLatin1String("    QString tableName() const { return QLatin1String(\"");
    output += tableName;
    output += QLatin1String("\"); }\n\n");

    // Property macros part
    output += QLatin1String("private:    /*** Don't modify below this line ***/\n    Q_OBJECT\n");
    it.toFront();
    while (it.hasNext()) {
        const QPair<QString, QString> &p = it.next();
        output += QString(SQLOBJECT_PROPERTY_TEMPLATE).arg(p.second, p.first);
    }

    // Footer part
    output += QString(SQLOBJECT_FOOTER_TEMPLATE).arg(modelName.toUpper());

    // Writes to a file
    QDir dst(dstDir.filePath("sqlobjects"));
    FileWriter fw(dst.filePath(modelName.toLower() + "object.h"));
    fw.write(output, false);
    return QLatin1String("sqlobjects/") + fw.fileName();
}


QStringList ModelGenerator::genModel() const
{
    QStringList ret;

    QPair<QStringList, QStringList> p = createModelParams();
    QString fileName = dstDir.filePath(modelName.toLower() + ".h");
    gen(fileName, MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QFileInfo(fileName).fileName();

    fileName = dstDir.filePath(modelName.toLower() + ".cpp");
    gen(fileName, MODEL_IMPL_TEMPLATE, p.second);
    ret << QFileInfo(fileName).fileName();
    return ret;
}


QStringList ModelGenerator::genUserModel(const QString &usernameField, const QString &passwordField) const
{
    QStringList ret;
    QPair<QStringList, QStringList> p = createModelParams();
    QString fileName = dstDir.filePath(modelName.toLower() + ".h");
    QString userVar = fieldNameToVariableName(usernameField);
    p.first << userVar << fieldNameToVariableName(passwordField);
    p.first << QLatin1String("    QString ") + USER_VIRTUAL_METHOD + "() const { return " + userVar + "(); }\n";

    gen(fileName, USER_MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << fileName;

    fileName = dstDir.filePath(modelName.toLower() + ".cpp");
    p.second << fieldNameToVariableName(usernameField) << fieldNameToVariableName(passwordField)
             << fieldNameToEnumName(usernameField);
    gen(fileName, USER_MODEL_IMPL_TEMPLATE, p.second);
    ret << fileName;
    return ret;
}


QPair<QStringList, QStringList> ModelGenerator::createModelParams() const
{
    QString setgetDecl;
    QString setgetImpl;
    QString crtparams;
    QString getOptDecl;
    QString getOptImpl;
    QString initParams;
    QList<QPair<QString, QString> > writableFields;
    bool optlockMethod = false;
    TableSchema ts(tableName);
    QList<QPair<QString, QString> > fieldList = ts.getFieldList();
    QString autoFieldName = ts.autoValueFieldName();

    for (QListIterator<QPair<QString, QString> > it(fieldList); it.hasNext(); ) {
        const QPair<QString, QString> &p = it.next();
        QString var = fieldNameToVariableName(p.first);

        // Getter method
        setgetDecl += QString("    %1 %2() const;\n").arg(p.second, var);
        setgetImpl += QString("%1 %2::%3() const\n{\n    return d->%4;\n}\n\n").arg(p.second, modelName, var, p.first);

        if (!excludedSetter()->contains(p.first, Qt::CaseInsensitive) && p.first != autoFieldName) {
            // Setter method
            QString str = fieldNameToEnumName(p.first);
            setgetDecl += QString("    void set%1(%2);\n").arg(str, createParam(p.second, p.first));
            setgetImpl += QString("void %1::set%2(%3)\n{\n    d->%4 = %5;\n}\n\n").arg(modelName, str, createParam(p.second, p.first), p.first, var);

            // Appends to crtparams-string
            crtparams += createParam(p.second, p.first);
            crtparams += ", ";

            writableFields << QPair<QString, QString>(p.first, p.second);

            if (p.second == "int" || p.second == "float" || p.second == "double") {
                initParams += QString("\n    d->") + p.first + " = 0;";
            }
        }

        if (p.first == LOCK_REVISION_FIELD)
            optlockMethod = true;
    }
    crtparams.chop(2);

    initParams += (initParams.isEmpty()) ? ' ' : '\n';

    // Creates parameters of get() method
    int idx = ts.primaryKeyIndex();
    QString getparams;
    if (idx < 0) {
        getparams = crtparams;
    } else {
        const QPair<QString, QString> &pair = fieldList[idx];
        getparams = createParam(pair.second, pair.first);
    }

    // Creates a declaration and a implementation of 'get' method for optimistic lock
    if (idx >= 0 &&optlockMethod) {
        const QPair<QString, QString> &pair = fieldList[idx];
        getOptDecl = QString("    static %1 get(%2, int lockRevision);\n").arg(modelName, createParam(pair.second, pair.first));

        getOptImpl = QString("%1 %1::get(%2, int lockRevision)\n"       \
                             "{\n"                                      \
                             "    TSqlORMapper<%1Object> mapper;\n"     \
                             "    TCriteria cri;\n"                     \
                             "    cri.add(%1Object::%3, %4);\n"         \
                             "    cri.add(%1Object::LockRevision, lockRevision);\n" \
                             "    return %1(mapper.findFirst(cri));\n"  \
                             "}\n\n").arg(modelName, createParam(pair.second, pair.first), fieldNameToEnumName(pair.first), fieldNameToVariableName(pair.first));
    }

    QStringList headerArgs;
    headerArgs << modelName.toUpper() << modelName << setgetDecl << crtparams << getparams << getOptDecl;
#if QT_VERSION >= 0x050000
    headerArgs << "class QJsonArray;\n" << "    static QJsonArray getAllJson();\n";
#else
    headerArgs << "" << "";
#endif

    // Creates a model implementation
    QString createImpl;
    createImpl += QString("    %1Object obj;\n").arg(modelName);

    QListIterator<QPair<QString, QString> > fi(writableFields);
    while (fi.hasNext()) {
        const QPair<QString, QString> &p = fi.next();
        createImpl += QString("    obj.%1 = %2;\n").arg(p.first, fieldNameToVariableName(p.first));
    }
    createImpl += "    if (!obj.create()) {\n";
    createImpl += "        obj.clear();\n";
    createImpl += "    }\n";
    createImpl += QString("    return %1(obj);\n").arg(modelName);

    // Creates a implementation of get() method
    QString getImpl;
    if (idx < 0) {
        // If no primary index exists
        getImpl += QString("    TCriteria cri;\n");
        fi.toFront();
        while (fi.hasNext()) {
            const QPair<QString, QString> &p = fi.next();
            getImpl += QString("    cri.add(%1Object::%2, %3);\n").arg(modelName, fieldNameToEnumName(p.first), fieldNameToVariableName(p.first));
        }
    }

    getImpl += QString("    TSqlORMapper<%1Object> mapper;\n").arg(modelName);
    getImpl += QString("    return %1(mapper.").arg(modelName);

    if (idx < 0) {
        getImpl += "findFirst(cri));\n";
    } else {
        const QPair<QString, QString> &pair = fieldList[idx];
        getImpl += QString("findByPrimaryKey(%1));\n").arg(fieldNameToVariableName(pair.first));
    }

    QStringList implArgs;
    implArgs << modelName.toLower() << modelName << initParams << setgetImpl << crtparams << createImpl << getparams << getImpl << getOptImpl;
#if QT_VERSION >= 0x050000
    implArgs << QString(MODEL_IMPL_GETALLJSON).arg(modelName);
#else
    implArgs << "";
#endif

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


QString ModelGenerator::createParam(const QString &type, const QString &name)
{
    QString string;
    QString var = fieldNameToVariableName(name);
    if (type == "int" || type == "float" || type == "double") {
        string.append(type).append(' ').append(var);
    } else {
        string += QString("const %1 &%2").arg(type, var);
    }
    return string;
}
