/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "servicegenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "modelgenerator.h"
#include "tableschema.h"
#include <tfnamespace.h>
#include <QtCore>


constexpr auto SERVICE_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                              "#include <TGlobal>\n"
                                              "\n"
                                              "class THttpRequest;\n"
                                              "class TSession;\n"
                                              "\n\n"
                                              "class T_MODEL_EXPORT %clsname%Service {\n"
                                              "public:\n"
                                              "    void index();\n"
                                              "    void show(%arg%);\n"
                                              "    %type% create(THttpRequest &request);\n"
                                              "    void edit(TSession &session, %arg%);\n"
                                              "    int save(THttpRequest &request, TSession &session, %arg%);\n"
                                              "    bool remove(%arg%);\n"
                                              "};\n"
                                              "\n";


constexpr auto SERVICE_SOURCE_FILE_TEMPLATE = "#include \"%name%service.h\"\n"
                                              "#include \"objects/%name%.h\"\n"
                                              "#include <TreeFrogModel>\n"
                                              "\n\n"
                                              "void %clsname%Service::index()\n"
                                              "{\n"
                                              "    auto %varname%List = %clsname%::getAll();\n"
                                              "    texport(%varname%List);\n"
                                              "}\n"
                                              "\n"
                                              "void %clsname%Service::show(%arg%)\n"
                                              "{\n"
                                              "    auto %varname% = %clsname%::get(%id%);\n"
                                              "    texport(%varname%);\n"
                                              "}\n"
                                              "\n"
                                              "%type% %clsname%Service::create(THttpRequest &request)\n"
                                              "{\n"
                                              "    auto %varname% = request.formItems(\"%varname%\");\n"
                                              "    auto model = %clsname%::create(%varname%);\n"
                                              "\n"
                                              "    if (model.isNull()) {\n"
                                              "        QString error = \"Failed to create.\";\n"
                                              "        texport(error);\n"
                                              "        texport(%varname%);\n"
                                              "        return %erres%;\n"
                                              "    }\n"
                                              "\n"
                                              "    QString notice = \"Created successfully.\";\n"
                                              "    tflash(notice);\n"
                                              "    return model.%id%();\n"
                                              "}\n"
                                              "\n"
                                              "void %clsname%Service::edit(TSession& session, %arg%)\n"
                                              "{\n"
                                              "    auto model = %clsname%::get(%id%);\n"
                                              "    if (!model.isNull()) {\n"
                                              "%code1%"
                                              "        auto %varname% = model.toVariantMap();\n"
                                              "        texport(%varname%);\n"
                                              "    }\n"
                                              "}\n"
                                              "\n"
                                              "int %clsname%Service::save(THttpRequest &request, TSession &session, %arg%)\n"
                                              "{\n"
                                              "%code2%"
                                              "    auto model = %clsname%::get(%id%%rev%);\n"
                                              "    \n"
                                              "    if (model.isNull()) {\n"
                                              "        QString error = \"Original data not found. It may have been updated/removed by another transaction.\";\n"
                                              "        tflash(error);\n"
                                              "        return 0;\n"
                                              "    }\n"
                                              "\n"
                                              "    auto %varname% = request.formItems(\"%varname%\");\n"
                                              "    model.setProperties(%varname%);\n"
                                              "    if (!model.save()) {\n"
                                              "        texport(%varname%);\n"
                                              "        QString error = \"Failed to update.\";\n"
                                              "        texport(error);\n"
                                              "        return -1;\n"
                                              "    }\n"
                                              "\n"
                                              "    QString notice = \"Updated successfully.\";\n"
                                              "    tflash(notice);\n"
                                              "    return 1;\n"
                                              "}\n"
                                              "\n"
                                              "bool %clsname%Service::remove(%arg%)\n"
                                              "{\n"
                                              "    auto %varname% = %clsname%::get(%id%);\n"
                                              "    return %varname%.remove();\n"
                                              "}\n"
                                              "\n";

class ErrorValue : public QHash<int, QString> {
public:
    ErrorValue() :
        QHash<int, QString>()
    {
        insert(QMetaType::Int, "-1");
        insert(QMetaType::UInt, "-1");
        insert(QMetaType::LongLong, "-1");
        insert(QMetaType::ULongLong, "-1");
        insert(QMetaType::Double, "-1");
        insert(QMetaType::QByteArray, "QByteArray()");
        insert(QMetaType::QString, "QString()");
        insert(QMetaType::QDate, "QDate()");
        insert(QMetaType::QTime, "QTime()");
        insert(QMetaType::QDateTime, "QDateTime()");
    }
};
Q_GLOBAL_STATIC(ErrorValue, errorValue)


ServiceGenerator::ServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx) :
    _serviceName(service), _fieldList(fields), _pkIndex(pkIdx), _lockRevIndex(lockRevIdx)
{
}


bool ServiceGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir);
    QStringList files;
    FileWriter fwh(dir.filePath(_serviceName.toLower() + "service.h"));
    FileWriter fws(dir.filePath(_serviceName.toLower() + "service.cpp"));

    if (_fieldList.isEmpty()) {
        qCritical("Incorrect parameters.");
        return false;
    }

    QPair<QString, QMetaType::Type> pair;
    if (_pkIndex < 0) {
        return false;
    } else {
        pair = _fieldList[_pkIndex];
    }

    // Generates a controller source code
    QString sessInsertStr;
    QString sessGetStr;
    QString revStr;
    QString varName = enumNameToVariableName(_serviceName);

    if (_lockRevIndex >= 0) {
        sessInsertStr = QString("        session.insert(\"%1_lockRevision\", model.lockRevision());\n").arg(varName);
        sessGetStr = QString("    int rev = session.value(\"%1_lockRevision\").toInt();\n").arg(varName);
        revStr = QLatin1String(", rev");
    }

    PlaceholderList replaceList = {
        {"name", _serviceName.toLower()},
        {"clsname", _serviceName},
        {"varname", varName},
        {"arg", ModelGenerator::createParam(pair.second, pair.first)},
        {"id", fieldNameToVariableName(pair.first)},
        {"type", QString::fromLatin1(QMetaType::typeName(pair.second))},
        {"code1", sessInsertStr},
        {"code2", sessGetStr},
        {"rev", revStr},
        {"erres", errorValue()->value(pair.second)},
    };

    // Generates a service header file
    QString code = replaceholder(headerFileTemplate(), replaceList);
    fwh.write(code, false);
    files << fwh.fileName();

    // Generates a service source file
    code = replaceholder(sourceFileTemplate(), replaceList);
    fws.write(code, false);
    files << fws.fileName();

    // Generates a project file
    ProjectFileGenerator progen(dir.filePath("models.pro"));
    return progen.add(files);
}


QString ServiceGenerator::headerFileTemplate() const
{
    return QLatin1String(SERVICE_HEADER_FILE_TEMPLATE);
}


QString ServiceGenerator::sourceFileTemplate() const
{
    return QLatin1String(SERVICE_SOURCE_FILE_TEMPLATE);
}
