/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "validatorgenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"

constexpr auto VALIDATOR_HEADER_TEMPLATE = "#pragma once\n"
                                           "#include <TGlobal>\n"
                                           "#include <TFormValidator>\n"
                                           "\n\n"
                                           "class T_HELPER_EXPORT %1Validator : public TFormValidator {\n"
                                           "public:\n"
                                           "    %1Validator();\n"
                                           "};\n"
                                           "\n"
                                           "Q_DECLARE_METATYPE(%1Validator)\n"
                                           "\n";

constexpr auto VALIDATOR_IMPL_TEMPLATE = "#include \"%1validator.h\"\n"
                                         "\n\n"
                                         "%2Validator::%2Validator() : TFormValidator()\n"
                                         "{\n"
                                         "    //Set the rules below\n"
                                         "    //setRule(\"xxxx\", Tf::MaxLength, 20);\n"
                                         "    //  :\n"
                                         "}\n";


ValidatorGenerator::ValidatorGenerator(const QString &validator)
{
    name = fieldNameToEnumName(validator);
    name.remove(QRegularExpression("validator$", QRegularExpression::CaseInsensitiveOption));
}


bool ValidatorGenerator::generate(const QString &dst) const
{
    // Writes each files
    QDir dstDir(dst);
    QString output = QString(VALIDATOR_HEADER_TEMPLATE).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "validator.h")).write(output, false);

    output = QString(VALIDATOR_IMPL_TEMPLATE).arg(name.toLower()).arg(name);
    FileWriter(dstDir.filePath(name.toLower() + "validator.cpp")).write(output, false);

    // Updates the project file
    ProjectFileGenerator progen(dstDir.filePath("helpers.pro"));
    QStringList files;
    files << name.toLower() + "validator.h" << name.toLower() + "validator.cpp";
    return progen.add(files);
}
