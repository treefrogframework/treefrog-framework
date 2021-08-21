/* Copyright (c) 2010-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "apiservicegenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "modelgenerator.h"
#include "tableschema.h"
#include <tfnamespace.h>
#include <QtCore>


constexpr auto SERVICE_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                              "#include <TGlobal>\n"
                                              "#include <QJsonObject>\n"
                                              "\n"
                                              "class THttpRequest;\n"
                                              "class TSession;\n"
                                              "\n\n"
                                              "class T_MODEL_EXPORT Api%clsname%Service {\n"
                                              "public:\n"
                                              "    QJsonObject index();\n"
                                              "    QJsonObject get(%arg%);\n"
                                              "    QJsonObject create(THttpRequest &request);\n"
                                              "    QJsonObject save(THttpRequest &request, %arg%);\n"
                                              "    QJsonObject remove(%arg%);\n"
                                              "};\n"
                                              "\n";


constexpr auto SERVICE_SOURCE_FILE_TEMPLATE = "#include \"api%name%service.h\"\n"
                                              "#include \"objects/%name%.h\"\n"
                                              "#include <TreeFrogModel>\n"
                                              "\n\n"
                                              "QJsonObject Api%clsname%Service::index()\n"
                                              "{\n"
                                              "    auto %varname%List = %clsname%::getAll();\n"
                                              "    QJsonObject json = {{\"data\", tfConvertToJsonArray(%varname%List)}};\n"
                                              "    return json;\n"
                                              "}\n"
                                              "\n"
                                              "QJsonObject Api%clsname%Service::get(%arg%)\n"
                                              "{\n"
                                              "    auto %varname% = %clsname%::get(%id%);\n"
                                              "    QJsonObject json = {{\"data\", %varname%.toJsonObject()}};\n"
                                              "    return json;\n"
                                              "}\n"
                                              "\n"
                                              "QJsonObject Api%clsname%Service::create(THttpRequest &request)\n"
                                              "{\n"
                                              "    QJsonObject json;\n"
                                              "    auto %varname% = request.jsonData().toVariant().toMap();\n"
                                              "    auto model = %clsname%::create(%varname%);\n"
                                              "\n"
                                              "    if (model.isNull()) {\n"
                                              "        json.insert(\"error\", QJsonObject({{\"message\", \"Internal Server Error\"}}));\n"
                                              "    } else {\n"
                                              "        json.insert(\"data\", model.toJsonObject());\n"
                                              "    }\n"
                                              "    return json;\n"
                                              "}\n"
                                              "\n"
                                              "QJsonObject Api%clsname%Service::save(THttpRequest &request, %arg%)\n"
                                              "{\n"
                                              "    QJsonObject json;\n"
                                              "    auto model = %clsname%::get(%id%);\n"
                                              "\n"
                                              "    if (model.isNull()) {\n"
                                              "        json.insert(\"error\", QJsonObject({{\"message\", \"Not found\"}}));\n"
                                              "        return json;\n"
                                              "    }\n"
                                              "\n"
                                              "    auto %varname% = request.jsonData();\n"
                                              "    model.setProperties(%varname%);\n"
                                              "\n"
                                              "    if (!model.save()) {\n"
                                              "        json.insert(\"error\", QJsonObject({{\"message\", \"Internal Server Error\"}}));\n"
                                              "    } else {\n"
                                              "        json.insert(\"data\", model.toJsonObject());\n"
                                              "    }\n"
                                              "    return json;\n"
                                              "}\n"
                                              "\n"
                                              "QJsonObject Api%clsname%Service::remove(%arg%)\n"
                                              "{\n"
                                              "    QJsonObject json;\n"
                                              "    auto %varname% = %clsname%::get(%id%);\n"
                                              "\n"
                                              "    if (%varname%.remove()) {\n"
                                              "        json.insert(\"status\", \"OK\");\n"
                                              "    }\n"
                                              "    return json;\n"
                                              "}\n"
                                              "\n";


ApiServiceGenerator::ApiServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx) :
    _serviceName(service), _fieldList(fields), _pkIndex(pkIdx)
{
}


bool ApiServiceGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir);
    QStringList files;
    FileWriter fwh(dir.filePath(QLatin1String("api") + _serviceName.toLower() + "service.h"));
    FileWriter fws(dir.filePath(QLatin1String("api") +_serviceName.toLower() + "service.cpp"));

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

    PlaceholderList replaceList = {
        {"name", _serviceName.toLower()},
        {"clsname", _serviceName},
        {"var", fieldNameToVariableName(pair.first)},
        {"varname", enumNameToVariableName(_serviceName)},
        {"arg", ModelGenerator::createParam(pair.second, pair.first)},
        {"id", fieldNameToVariableName(pair.first)},
    };

    // Generates a service header file
    QString code = replaceholder(SERVICE_HEADER_FILE_TEMPLATE, replaceList);
    fwh.write(code, false);
    files << fwh.fileName();

    // Generates a controller source code
    code = replaceholder(SERVICE_SOURCE_FILE_TEMPLATE, replaceList);
    fws.write(code, false);
    files << fws.fileName();

    // Generates a project file
    ProjectFileGenerator progen(dir.filePath("models.pro"));
    return progen.add(files);
}
