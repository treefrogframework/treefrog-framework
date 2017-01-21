/* Copyright (c) 2016-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "helpergenerator.h"
#include "global.h"
#include "filewriter.h"
#include "projectfilegenerator.h"

#define HELPER_HEADER_TEMPLATE                                          \
    "#ifndef %1_H\n"                                                    \
    "#define %1_H\n"                                                    \
    "\n"                                                                \
    "#include <TGlobal>\n"                                              \
    "#include \"applicationhelper.h\"\n"                                \
    "\n"                                                                \
    "class T_HELPER_EXPORT %2 : public ApplicationHelper\n"             \
    "{\n"                                                               \
    "public:\n"                                                         \
    "    %2();\n"                                                       \
    "};\n"                                                              \
    "\n"                                                                \
    "#endif // %1_H\n"

#define HELPER_IMPL_TEMPLATE                            \
    "#include \"%1.h\"\n"                               \
    "\n"                                                \
    "%2::%2() : ApplicationHelper()\n"                  \
    "{ }\n"


HelperGenerator::HelperGenerator(const QString &n)
{
    name = fieldNameToEnumName(n);
}


bool HelperGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(HELPER_HEADER_TEMPLATE).arg(name.toUpper()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + ".h")).write(output, false);

    output = QString(HELPER_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + ".cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("helpers.pro"));
    QStringList files;
    files << name.toLower() + ".h" << name.toLower() + ".cpp";
    return progen.add(files);
}
