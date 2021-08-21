/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "apicontrollergenerator.h"
#include "controllergenerator.h"
#include "filewriter.h"
#include "global.h"
#include "modelgenerator.h"
#include "projectfilegenerator.h"
#include "tableschema.h"
#include <QtCore>

constexpr auto CONTROLLER_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                                 "#include \"applicationcontroller.h\"\n"
                                                 "\n\n"
                                                 "class T_CONTROLLER_EXPORT Api%clsname%Controller : public ApplicationController {\n"
                                                 "    Q_OBJECT\n"
                                                 "public slots:\n"
                                                 "    void index();\n"
                                                 "    void get(const QString &%id%);\n"
                                                 "    void create();\n"
                                                 "    void save(const QString &%id%);\n"
                                                 "    void remove(const QString &%id%);\n"
                                                 "};\n"
                                                 "\n";


constexpr auto CONTROLLER_SOURCE_FILE_TEMPLATE = "#include \"api%name%controller.h\"\n"
                                                 "#include \"api%name%service.h\"\n"
                                                 "\n"
                                                 "static Api%clsname%Service service;\n"
                                                 "\n\n"
                                                 "void Api%clsname%Controller::index()\n"
                                                 "{\n"
                                                 "    auto json = service.index();\n"
                                                 "    renderJson(json);\n"
                                                 "}\n"
                                                 "\n"
                                                 "void Api%clsname%Controller::get(const QString &%id%)\n"
                                                 "{\n"
                                                 "    auto json = service.get(%var1%);\n"
                                                 "    renderJson(json);\n"
                                                 "}\n"
                                                 "\n"
                                                 "void Api%clsname%Controller::create()\n"
                                                 "{\n"
                                                 "    QJsonObject json;\n"
                                                 "\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Post:\n"
                                                 "        json = service.create(request());\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        setStatusCode(Tf::BadRequest);\n"
                                                 "        break;\n"
                                                 "    }\n"
                                                 "    renderJson(json);\n"
                                                 "}\n"
                                                 "\n"
                                                 "void Api%clsname%Controller::save(const QString &%id%)\n"
                                                 "{\n"
                                                 "    QJsonObject json;\n"
                                                 "\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Post:\n"
                                                 "        json = service.save(request(), %var1%);\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        setStatusCode(Tf::BadRequest);\n"
                                                 "        break;\n"
                                                 "    }\n"
                                                 "    renderJson(json);\n"
                                                 "}\n"
                                                 "\n"
                                                 "void Api%clsname%Controller::remove(const QString &%id%)\n"
                                                 "{\n"
                                                 "    QJsonObject json;\n"
                                                 "\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Post:\n"
                                                 "        json = service.remove(%var1%);\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        setStatusCode(Tf::BadRequest);\n"
                                                 "        return;\n"
                                                 "    }\n"
                                                 "    renderJson(json);\n"
                                                 "}\n"
                                                 "\n"
                                                 "// Don't remove below this line\n"
                                                 "T_DEFINE_CONTROLLER(Api%clsname%Controller)\n";


class NGCtlrName : public QStringList {
public:
    NGCtlrName() :
        QStringList()
    {
        append("layouts");
        append("partial");
        append("direct");
        append("_src");
        append("mailer");
    }
};
Q_GLOBAL_STATIC(NGCtlrName, ngCtlrName)


ApiControllerGenerator::ApiControllerGenerator(const QString &controller, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx) :
    _controllerName(controller), _fieldList(fields), _pkIndex(pkIdx)
{
}


bool ApiControllerGenerator::generate(const QString &dstDir) const
{
    // Reserved word check
    if (ngCtlrName()->contains(_tableName.toLower())) {
        qCritical("Reserved word error. Please use another word.  Controller name: %s", qPrintable(_tableName));
        return false;
    }

    QDir dir(dstDir);
    QStringList files;
    FileWriter fwh(dir.filePath(QLatin1String("api") + _controllerName.toLower() + "controller.h"));
    FileWriter fws(dir.filePath(QLatin1String("api") + _controllerName.toLower() + "controller.cpp"));

    if (_fieldList.isEmpty()) {
        qCritical("Incorrect parameters.");
        return false;
    }

    QPair<QString, QMetaType::Type> pair;
    if (_pkIndex >= 0) {
        pair = _fieldList[_pkIndex];
    } else {
        return false;
    }

    PlaceholderList replaceList = {
        {"name", _controllerName.toLower()},
        {"clsname", _controllerName},
        {"var1", ControllerGenerator::generateIdString(pair.first, pair.second)},
        {"arg", ModelGenerator::createParam(pair.second, pair.first)},
        {"id", fieldNameToVariableName(pair.first)},
    };

    // Generates a API controller header file
    QString code = replaceholder(CONTROLLER_HEADER_FILE_TEMPLATE, replaceList);
    fwh.write(code, false);
    files << fwh.fileName();

    // Generates a API controller source file
    code = replaceholder(CONTROLLER_SOURCE_FILE_TEMPLATE, replaceList);
    fws.write(code, false);
    files << fws.fileName();

    // Generates a project file
    ProjectFileGenerator progen(dir.filePath("controllers.pro"));
    return progen.add(files);
}
