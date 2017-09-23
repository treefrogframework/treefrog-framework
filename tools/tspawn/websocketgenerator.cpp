/* Copyright (c) 2015-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "websocketgenerator.h"
#include "global.h"
#include "filewriter.h"
#include "projectfilegenerator.h"

#define ENDPOINT_HEADER_TEMPLATE                                        \
    "#ifndef %1ENDPOINT_H\n"                                            \
    "#define %1ENDPOINT_H\n"                                            \
    "\n"                                                                \
    "#include \"applicationendpoint.h\"\n"                              \
    "\n"                                                                \
    "class T_CONTROLLER_EXPORT %2Endpoint : public ApplicationEndpoint\n" \
    "{\n"                                                               \
    "    Q_OBJECT\n"                                                    \
    "public:\n"                                                         \
    "    %2Endpoint();\n"                                               \
    "    %2Endpoint(const %2Endpoint &other);\n"                        \
    "\n"                                                                \
    "protected:\n"                                                      \
    "    bool onOpen(const TSession &httpSession) override;\n"          \
    "    void onClose(int closeCode) override;\n"                       \
    "    void onTextReceived(const QString &text) override;\n"          \
    "    void onBinaryReceived(const QByteArray &binary) override;\n"   \
    "};\n"                                                              \
    "\n"                                                                \
    "#endif // %1ENDPOINT_H\n"

#define ENDPOINT_IMPL_TEMPLATE                                          \
    "#include \"%1endpoint.h\"\n"                                       \
    "\n"                                                                \
    "%2Endpoint::%2Endpoint()\n"                                        \
    "    : ApplicationEndpoint()\n"                                     \
    "{ }\n"                                                             \
    "\n"                                                                \
    "%2Endpoint::%2Endpoint(const %2Endpoint &)\n"                      \
    "    : ApplicationEndpoint()\n"                                     \
    "{ }\n"                                                             \
    "\n"                                                                \
    "bool %2Endpoint::onOpen(const TSession &)\n"                       \
    "{\n"                                                               \
    "    return true;\n"                                                \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Endpoint::onClose(int)\n"                                   \
    "{ }\n"                                                             \
    "\n"                                                                \
    "void %2Endpoint::onTextReceived(const QString &)\n"                \
    "{\n"                                                               \
    "    // write code\n"                                               \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Endpoint::onBinaryReceived(const QByteArray &)\n"           \
    "{ }\n"                                                             \
    "\n\n"                                                              \
    "// Don't remove below this line\n"                                 \
    "T_DEFINE_CONTROLLER(%2Endpoint)\n"


WebSocketGenerator::WebSocketGenerator(const QString &n)
{
    name = fieldNameToEnumName(n);
    name.remove(QRegExp("endpoint$", Qt::CaseInsensitive));
}


bool WebSocketGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(ENDPOINT_HEADER_TEMPLATE).arg(name.toUpper()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "endpoint.h")).write(output, false);

    output = QString(ENDPOINT_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "endpoint.cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("controllers.pro"));
    QStringList files;
    files << name.toLower() + "endpoint.h" << name.toLower() + "endpoint.cpp";
    return progen.add(files);
}
