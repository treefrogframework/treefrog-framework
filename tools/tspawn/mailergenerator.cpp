/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "mailergenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"
#include <QtCore>

constexpr auto MAILER_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                             "#include <TActionMailer>\n"
                                             "\n\n"
                                             "class %1Mailer : public TActionMailer {\n"
                                             "public:\n"
                                             "    %1Mailer() { }\n"
                                             "%2"
                                             "};\n"
                                             "\n";


constexpr auto MAILER_SOURCE_FILE_TEMPLATE = "#include \"%1mailer.h\"\n"
                                             "\n\n"
                                             "%2\n";


MailerGenerator::MailerGenerator(const QString &name, const QStringList &actions) :
    actionList(actions)
{
    mailerName = fieldNameToEnumName(name);
}


bool MailerGenerator::generate(const QString &dst) const
{
    if (actionList.isEmpty()) {
        qCritical("Bad parameter: Action name empty");
        return false;
    }

    QDir dstDir(dst);
    QStringList files;
    FileWriter fwh(dstDir.filePath(mailerName.toLower() + "mailer.h"));
    FileWriter fws(dstDir.filePath(mailerName.toLower() + "mailer.cpp"));

    // Generates a mailer header file
    QString act;
    for (QStringListIterator i(actionList); i.hasNext();) {
        act.append("    void ").append(i.next()).append("();\n");
    }

    QString code = QString(MAILER_HEADER_FILE_TEMPLATE).arg(mailerName, act);
    fwh.write(code, false);
    files << fwh.fileName();

    // Generates a mailer source code
    QString actimpl;
    for (QStringListIterator i(actionList); i.hasNext();) {
        actimpl.append("void ").append(mailerName).append("Mailer::").append(i.next()).append("()\n{\n    //\n    // write code\n    //\n\n    deliver(\"mail\");\n}\n\n");
    }
    code = QString(MAILER_SOURCE_FILE_TEMPLATE).arg(mailerName.toLower(), actimpl);
    fws.write(code, false);
    files << fws.fileName();

    // Updates a project file
    ProjectFileGenerator progen(dstDir.filePath("controllers.pro"));
    return progen.add(files);
}
