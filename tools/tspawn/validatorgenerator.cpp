/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "validatorgenerator.h"
#include "global.h"
#include "filewriter.h"
#include "projectfilegenerator.h"

#define VALIDATOR_HEADER_TEMPLATE                                       \
    "#ifndef %1VALIDATOR_H\n"                                           \
    "#define %1VALIDATOR_H\n"                                           \
    "\n"                                                                \
    "#include <TGlobal>\n"                                              \
    "#include <TFormValidator>\n"                                       \
    "\n"                                                                \
    "class T_HELPER_EXPORT %2Validator : public TFormValidator\n"       \
    "{\n"                                                               \
    "public:\n"                                                         \
    "    %2Validator();\n"                                              \
    "};\n"                                                              \
    "\n"                                                                \
    "Q_DECLARE_METATYPE(%2Validator)\n"                                 \
    "\n"                                                                \
    "#endif // %1VALIDATOR_H\n"

#define VALIDATOR_IMPL_TEMPLATE                         \
    "#include \"%1validator.h\"\n"                      \
    "\n"                                                \
    "%2Validator::%2Validator() : TFormValidator()\n"   \
    "{\n"                                               \
    "    // Set the rules below\n"                      \
    "    //setRule(\"xxxx\", Tf::MaxLength, 20);\n"     \
    "    //  :\n"                                       \
    "}\n"


ValidatorGenerator::ValidatorGenerator(const QString &validator)
{
    name = fieldNameToEnumName(validator);
    name.remove(QRegExp("validator$", Qt::CaseInsensitive));
}


bool ValidatorGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(VALIDATOR_HEADER_TEMPLATE).arg(name.toUpper()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "validator.h")).write(output, false);

    output = QString(VALIDATOR_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "validator.cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("helpers.pro"));
    QStringList files;
    files << name.toLower() + "validator.h" << name.toLower() + "validator.cpp";
    return progen.add(files);
}
