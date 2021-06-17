/* Copyright (c) 2016-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "helpergenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"

constexpr auto HELPER_HEADER_TEMPLATE = "#pragma once\n"
                                        "#include <TGlobal>\n"
                                        "#include \"applicationhelper.h\"\n"
                                        "\n\n"
                                        "class T_HELPER_EXPORT %1 : public ApplicationHelper {\n"
                                        "public:\n"
                                        "    %1();\n"
                                        "};\n"
                                        "\n";

constexpr auto HELPER_IMPL_TEMPLATE = "#include \"%1.h\"\n"
                                      "\n\n"
                                      "%2::%2() : ApplicationHelper()\n"
                                      "{ }\n";


HelperGenerator::HelperGenerator(const QString &n)
{
    name = fieldNameToEnumName(n);
}


bool HelperGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(HELPER_HEADER_TEMPLATE).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + ".h")).write(output, false);

    output = QString(HELPER_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + ".cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("helpers.pro"));
    QStringList files;
    files << name.toLower() + ".h" << name.toLower() + ".cpp";
    return progen.add(files);
}
