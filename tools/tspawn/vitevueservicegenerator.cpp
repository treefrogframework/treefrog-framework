/* Copyright (c) 2010-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "vitevueservicegenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "modelgenerator.h"
#include "tableschema.h"
#include <tfnamespace.h>
#include <QtCore>


constexpr auto SERVICE_HEADER_FILE_TEMPLATE =
    "#pragma once\n"
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

constexpr auto SERVICE_SOURCE_FILE_TEMPLATE =
    "#include \"%name%service.h\"\n"
    "#include \"objects/%name%.h\"\n"
    "#include <TreeFrogModel>\n"
    "\n\n"
    "void %clsname%Service::index()\n"
    "{\n"
    "    QJsonObject props {\n"
    "        {\"items\", %clsname%::getAllJson()}\n"
    "    };\n"
    "    texport(props);\n"
    "}\n"
    "\n"
    "void %clsname%Service::show(%arg%)\n"
    "{\n"
    "    auto model = %clsname%::get(%id%);\n"
    "    QJsonObject props {\n"
    "        {\"item\", model.toJsonObject()},\n"
    "        {\"error\", QString()},\n"
    "        {\"notice\", Tf::currentController()->flashVariant(\"notice\").toString()},\n"
    "    };\n"
    "\n"
    "    if (model.isNull()) {\n"
    "        props[\"error\"] = \"Data not found.\";\n"
    "    }\n"
    "    texport(props);\n"
    "}\n"
    "\n"
    "%type% %clsname%Service::create(THttpRequest &request)\n"
    "{\n"
    "    auto item = request.formItems(\"%varname%\");\n"
    "    auto model = %clsname%::create(item);\n"
    "\n"
    "    if (model.isNull()) {\n"
    "        QJsonObject props {\n"
    "            {\"item\", QJsonObject::fromVariantMap(item)},\n"
    "            {\"error\", \"Failed to create.\"},\n"
    "        };\n"
    "        texport(props);\n"
    "        return %erres%;  // render\n"
    "    }\n"
    "\n"
    "    QString notice = \"Created successfully.\";\n"
    "    tflash(notice);\n"
    "    return model.%id%();  // redirect to show\n"
    "}\n"
    "\n"
    "void %clsname%Service::edit(TSession& session, %arg%)\n"
    "{\n"
    "    QJsonObject props {\n"
    "        {\"item\", QJsonObject()},\n"
    "        {\"error\", Tf::currentController()->flashVariant(\"error\").toString()},\n"
    "    };\n"
    "\n"
    "    auto model = %clsname%::get(%id%);\n"
    "    if (!model.isNull()) {\n"
    "        props[\"item\"] = model.toJsonObject();\n"
    "%code1%"
    "    }\n"
    "    texport(props);  // render\n"
    "}\n"
    "\n"
    "int %clsname%Service::save(THttpRequest &request, TSession &session, %arg%)\n"
    "{\n"
    "%code2%"
    "    auto model = %clsname%::get(%id%%rev%);\n"
    "\n"
    "    if (model.isNull()) {\n"
    "        QString error = \"Original data not found. It may have been updated/removed by another transaction.\";\n"
    "        tflash(error);\n"
    "        return 0;  // redirect to save\n"
    "    }\n"
    "\n"
    "    auto item = request.formItems(\"%varname%\");\n"
    "    model.setProperties(item);\n"
    "    if (!model.save()) {\n"
    "        QJsonObject props {\n"
    "            {\"item\", QJsonObject::fromVariantMap(item)},\n"
    "            {\"error\", QString(\"Failed to update.\")},\n"
    "        };\n"
    "        texport(props);\n"
    "        return -1;  // render\n"
    "    }\n"
    "\n"
    "    QString notice = \"Updated successfully.\";\n"
    "    tflash(notice);\n"
    "    return 1;  // redirect to show\n"
    "}\n"
    "\n"
    "bool %clsname%Service::remove(%arg%)\n"
    "{\n"
    "    auto %varname% = %clsname%::get(%id%);\n"
    "    return %varname%.remove();\n"
    "}\n"
    "\n";


ViteVueServiceGenerator::ViteVueServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx) :
    ServiceGenerator(service, fields, pkIdx, lockRevIdx)
{
}


QString ViteVueServiceGenerator::headerFileTemplate() const
{
    return QLatin1String(SERVICE_HEADER_FILE_TEMPLATE);
}


QString ViteVueServiceGenerator::sourceFileTemplate() const
{
    return QLatin1String(SERVICE_SOURCE_FILE_TEMPLATE);
}
