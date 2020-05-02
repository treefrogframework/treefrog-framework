/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <TAppSettings>
#include <TTemporaryFile>
#include <TWebApplication>

/*!
  \class TTemporaryFile
  \brief The TTemporaryFile class is a temporary file device on an action
  context of web application. After an action of a controller returns,
  temporary files created in the action will be deleted. Because this
  class inherits QTemporaryFile, you can use same functions as that.
  \sa http://doc.qt.io/qt-5/qtemporaryfile.html
*/

/*!
  Constructor.
*/
TTemporaryFile::TTemporaryFile()
{
    QString tmppath;

    if (Tf::app()) {
        tmppath = Tf::appSettings()->value(Tf::UploadTemporaryDirectory).toString().trimmed();
        if (!tmppath.isEmpty() && QDir::isRelativePath(tmppath)) {
            tmppath = Tf::app()->webRootPath() + tmppath + "/";
        }

        if (!QDir(tmppath).exists()) {
            tmppath = "";
        }
    }

    if (tmppath.trimmed().isEmpty()) {
        tmppath = QDir::tempPath();
    }

    if (!tmppath.endsWith("/")) {
        tmppath += "/";
    }
    setFileTemplate(tmppath + QLatin1String("tf_temp.XXXXXXXXXXXXXXXX"));
}

/*!
  Returns an absolute path including the file name.
*/
QString TTemporaryFile::absoluteFilePath() const
{
    QFileInfo info(*this);
    return info.absoluteFilePath();
}

/*!
  Creates a unique file name for the temporary file, and opens it in
  QIODevice::ReadWrite mode. The file is guaranteed to have been created
  by this function (i.e., it has never existed before).
*/
bool TTemporaryFile::open()
{
    return QTemporaryFile::open();
}


bool TTemporaryFile::open(OpenMode flags)
{
    return QTemporaryFile::open(flags);
}
