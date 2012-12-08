/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <TTemporaryFile>
#include <TWebApplication>

/*!
  \class TTemporaryFile
  \brief The TTemporaryFile class is a temporary file device on an
  action context of web application.
*/

TTemporaryFile::TTemporaryFile()
{
    QString tmppath;
    if (Tf::app()) {
        tmppath = Tf::app()->appSettings().value("UploadTemporaryDirectory").toString().trimmed();
        if (!tmppath.isEmpty() && QDir::isRelativePath(tmppath)) {
            tmppath = Tf::app()->webRootPath() + tmppath + QDir::separator();
        }

        if (!QDir(tmppath).exists()) {
            tmppath = "";
        }
    }

    if (tmppath.trimmed().isEmpty()) {
        tmppath = QDir::tempPath();
    }

    if (!tmppath.endsWith(QDir::separator())) {
        tmppath += QDir::separator();
    }
    setFileTemplate(tmppath + "tf_temp.XXXXXXXXXXXXXXXX");
}


QString TTemporaryFile::absoluteFilePath() const
{
    QFileInfo info(*this);
    return info.absoluteFilePath();
}
