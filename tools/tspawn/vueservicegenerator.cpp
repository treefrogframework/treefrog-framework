/* Copyright (c) 2010-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "vueservicegenerator.h"
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
                                              "    auto items = %clsname%::getAllJson();\n"
                                              "    texport(items);\n"
                                              "}\n"
                                              "\n"
                                              "void %clsname%Service::show(%arg%)\n"
                                              "{\n"
                                              "    auto item = %clsname%::get(%id%).toJsonObject();\n"
                                              "    texport(item);\n"
                                              "}\n"
                                              "\n"
                                              "%type% %clsname%Service::create(THttpRequest &request)\n"
                                              "{\n"
                                              "    auto item = request.formItems(\"%varname%\");\n"
                                              "    auto model = %clsname%::create(item);\n"
                                              "\n"
                                              "    if (model.isNull()) {\n"
                                              "        QString error = \"Failed to create.\";\n"
                                              "        texport(error);\n"
                                              "        texport(item);\n"
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
                                              "        auto item = model.toJsonObject();\n"
                                              "        texport(item);\n"
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
                                              "    auto item = request.formItems(\"%varname%\");\n"
                                              "    model.setProperties(item);\n"
                                              "    if (!model.save()) {\n"
                                              "        texport(item);\n"
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


VueServiceGenerator::VueServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx) :
    ServiceGenerator(service, fields, pkIdx, lockRevIdx)
{
}


QString VueServiceGenerator::headerFileTemplate() const
{
    return QLatin1String(SERVICE_HEADER_FILE_TEMPLATE);
}


QString VueServiceGenerator::sourceFileTemplate() const
{
    return QLatin1String(SERVICE_SOURCE_FILE_TEMPLATE);
}
