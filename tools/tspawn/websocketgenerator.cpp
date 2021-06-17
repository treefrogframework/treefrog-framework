/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "websocketgenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"

constexpr auto ENDPOINT_HEADER_TEMPLATE = "#pragma once\n"
                                          "#include \"applicationendpoint.h\"\n"
                                          "\n\n"
                                          "class T_CONTROLLER_EXPORT %1Endpoint : public ApplicationEndpoint {\n"
                                          "    Q_OBJECT\n"
                                          "public:\n"
                                          "    %1Endpoint();\n"
                                          "    %1Endpoint(const %1Endpoint &other);\n"
                                          "\n"
                                          "protected:\n"
                                          "    bool onOpen(const TSession &httpSession) override;\n"
                                          "    void onClose(int closeCode) override;\n"
                                          "    void onTextReceived(const QString &text) override;\n"
                                          "    void onBinaryReceived(const QByteArray &binary) override;\n"
                                          "};\n"
                                          "\n";

constexpr auto ENDPOINT_IMPL_TEMPLATE = "#include \"%1endpoint.h\"\n"
                                        "\n\n"
                                        "%2Endpoint::%2Endpoint() :\n"
                                        "    ApplicationEndpoint()\n"
                                        "{ }\n"
                                        "\n"
                                        "%2Endpoint::%2Endpoint(const %2Endpoint &) :\n"
                                        "    ApplicationEndpoint()\n"
                                        "{ }\n"
                                        "\n"
                                        "bool %2Endpoint::onOpen(const TSession &)\n"
                                        "{\n"
                                        "    return true;\n"
                                        "}\n"
                                        "\n"
                                        "void %2Endpoint::onClose(int)\n"
                                        "{ }\n"
                                        "\n"
                                        "void %2Endpoint::onTextReceived(const QString &)\n"
                                        "{\n"
                                        "    // write code\n"
                                        "}\n"
                                        "\n"
                                        "void %2Endpoint::onBinaryReceived(const QByteArray &)\n"
                                        "{ }\n"
                                        "\n\n"
                                        "// Don't remove below this line\n"
                                        "T_DEFINE_CONTROLLER(%2Endpoint)\n";


WebSocketGenerator::WebSocketGenerator(const QString &n)
{
    name = fieldNameToEnumName(n);
    name.remove(QRegularExpression("endpoint$", QRegularExpression::CaseInsensitiveOption));
}


bool WebSocketGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(ENDPOINT_HEADER_TEMPLATE).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "endpoint.h")).write(output, false);

    output = QString(ENDPOINT_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "endpoint.cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("controllers.pro"));
    QStringList files;
    files << name.toLower() + "endpoint.h" << name.toLower() + "endpoint.cpp";
    return progen.add(files);
}
