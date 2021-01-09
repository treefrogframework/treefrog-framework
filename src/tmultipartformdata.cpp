/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <TActionContext>
#include <THttpRequest>
#include <THttpUtility>
#include <TMultipartFormData>
#include <TTemporaryFile>
#include <TWebApplication>
using namespace Tf;

const QFile::Permissions TMultipartFormData::DefaultPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther;
const QFile::Permissions TMimeEntity::DefaultPermissions = TMultipartFormData::DefaultPermissions;

/*!
  \class TMimeHeader
  \brief The TMimeHeader class contains MIME header information for internet.
*/

/*!
  Copy constructor.
*/
TMimeHeader::TMimeHeader(const TMimeHeader &other) :
    headers(other.headers)
{
}

/*!
  Assignment operator.
 */
TMimeHeader &TMimeHeader::operator=(const TMimeHeader &other)
{
    headers = other.headers;
    return *this;
}

/*!
  Returns the value of the header \a headerName.
*/
QByteArray TMimeHeader::header(const QByteArray &headerName) const
{
    QByteArray name = headerName.toLower();
    for (const auto &p : headers) {
        if (p.first.toLower() == name) {
            return p.second;
        }
    }
    return QByteArray();
}

/*!
  Sets the header \a headerName to be of value \a value.
*/
void TMimeHeader::setHeader(const QByteArray &headerName, const QByteArray &value)
{
    headers << qMakePair(headerName, value);
}

/*!
  Returns the value of the parameter \a name in the header field
  content-disposition.
*/
QByteArray TMimeHeader::contentDispositionParameter(const QByteArray &name) const
{
    QByteArray disp = header("content-disposition");
    QMap<QByteArray, QByteArray> params = parseHeaderParameter(disp);
    return params[name];
}

/*!
  Returns the value of the 'name' parameter in the header field
  content-disposition.
*/
QByteArray TMimeHeader::dataName() const
{
    return contentDispositionParameter("name");
}

/*!
  Returns the value of the 'filename' parameter in the header field
  content-disposition, indicating the original name of the file before
  uploading.
*/
QString TMimeHeader::originalFileName() const
{
    return QString::fromUtf8(contentDispositionParameter("filename").data());
}


int TMimeHeader::skipWhitespace(const QByteArray &text, int from)
{
    from = qMax(from, 0);
    while (from < text.length()) {
        char c = text[from];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
        ++from;
    }
    return from;
}

/*!
  Parses the MIME header \a header and returns the map of those headers.
  This function is for internal use only.
*/
QMap<QByteArray, QByteArray> TMimeHeader::parseHeaderParameter(const QByteArray &header)
{
    QMap<QByteArray, QByteArray> result;
    int pos = 0;

    for (;;) {
        pos = skipWhitespace(header, pos);
        if (pos >= header.length()) {
            return result;
        }
        int semicol = header.indexOf(';', pos);
        if (semicol < 0) {
            semicol = header.length();
        }

        QByteArray key;
        int equal = header.indexOf('=', pos);
        if (equal < 0 || equal > semicol) {
            key = header.mid(pos, semicol - pos).trimmed();
            if (!key.isEmpty()) {
                result.insert(key, QByteArray());
            }
            pos = semicol + 1;
            continue;
        }

        key = header.mid(pos, equal - pos).trimmed();
        pos = equal + 1;

        pos = skipWhitespace(header, pos);
        if (pos >= header.length()) {
            return result;
        }

        QByteArray value;
        if (header[pos] == '"') {
            ++pos;
            while (pos < header.length()) {
                char c = header.at(pos);
                if (c == '"') {
                    // end of quoted text
                    break;
                    // } else if (c == '\\') {
                    //     ++pos;
                    //     if (pos >= header.length()) {
                    //         // broken header
                    //         return result;
                    //     }
                    //     c = header[pos];
                }

                value += c;
                ++pos;
            }
        } else {
            while (pos < header.length()) {
                char c = header.at(pos);
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ';') {
                    break;
                }
                value += c;
                ++pos;
            }
        }

        result.insert(key, value);
    }
    return result;
}

/*!
  \class TMimeEntity
  \brief The TMimeEntity represents a MIME entity.
*/

/*!
  Copy constructor.
*/
TMimeEntity::TMimeEntity(const TMimeEntity &other) :
    entity(other.entity)
{
}

/*!
  Assignment operator.
 */
TMimeEntity &TMimeEntity::operator=(const TMimeEntity &other)
{
    entity = other.entity;
    return *this;
}

/*!
  Constructor with the header \a header and the body \a body.
*/
TMimeEntity::TMimeEntity(const TMimeHeader &header, const QString &body)
{
    entity.first = header;
    entity.second = body;
}

/*!
  Returns the value of the MIME header field content-type.
*/
QString TMimeEntity::contentType() const
{
    return header("content-type");
}

/*!
  Returns the file size in bytes. If the file does not exist,
  -1 is returned.
*/
qint64 TMimeEntity::fileSize() const
{
    QFileInfo fi(entity.second);
    if (!fi.exists()) {
        return -1;
    }
    return fi.size();
}

/*!
  Renames the file contained in this entity to \a newName.
  Returns true if successful; otherwise returns false.
  This function will overwrite it if the \a overwrite is true.
  The \a newName can have a relative path or an absolute path.
*/
bool TMimeEntity::renameUploadedFile(const QString &newName, bool overwrite, QFile::Permissions permissions)
{
    QString path = uploadedFilePath();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.exists()) {
        return false;
    }

    QString newpath = QDir::isAbsolutePath(newName) ? newName : Tf::app()->webRootPath() + newName;
    QFile newfile(newpath);
    if (newfile.exists()) {
        if (overwrite) {
            newfile.remove();
        } else {
            return false;
        }
    }

    file.setPermissions(permissions);
#ifdef Q_OS_WIN
    bool ret = file.copy(newpath);
    file.remove();  // maybe fail here, but will be removed after.
    return ret;
#else
    return file.rename(newpath);
#endif
}

/*!
  Returns the path of the temporary file contained in this entity,
  including the absolute path.
*/
QString TMimeEntity::uploadedFilePath() const
{
    // check original filename
    return (header().isEmpty() || header().contentDispositionParameter("filename").isEmpty()) ? QString() : entity.second;
}


/*!
  \class TMultipartFormData
  \brief The TMultipartFormData represents a media-type multipart/form-data.
*/

/*!
  Constructs a empty multipart/form-data object with the boundary
  \a boundary.
*/
TMultipartFormData::TMultipartFormData(const QByteArray &boundary) :
    dataBoundary(boundary)
{
}

/*!
  Constructs a multipart/form-data object by parsing \a formData with
  the boundary \a boundary.
*/
TMultipartFormData::TMultipartFormData(const QByteArray &formData, const QByteArray &boundary) :
    dataBoundary(boundary)
{
    QByteArray data(formData);
    QBuffer buffer(&data);
    parse(&buffer);
}

/*!
  Constructs a multipart/form-data object by parsing the content of
  the file with the given \a bodyFilePath.
*/
TMultipartFormData::TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary) :
    dataBoundary(boundary), bodyFile(bodyFilePath)
{
    QFile file(bodyFilePath);
    parse(&file);
}

/*!
  Returns true if the multipart/form-data object has no data;
  otherwise returns false.
*/
bool TMultipartFormData::isEmpty() const
{
    return postParameters.isEmpty() && uploadedFiles.isEmpty();
}

bool TMultipartFormData::hasFormItem(const QString &name) const
{
    return THttpRequest::hasItem(name, postParameters);
}

QString TMultipartFormData::formItemValue(const QString &name) const
{
    return THttpRequest::itemValue(name, QString(), postParameters);
}

/*!
    Clears this data.
 */
void TMultipartFormData::clear()
{
    dataBoundary.resize(0);
    postParameters.clear();
    uploadedFiles.clear();
}

/*!
  Returns a list of form string values whose name is equal to \a name
  from the multipart/form-data.
 */
QStringList TMultipartFormData::allFormItemValues(const QString &name) const
{
    return THttpRequest::allItemValues(name, postParameters);
}

/*
  Returns a list of QVariant values whose name is equal to \a name
  from the multipart/form-data.
  */
QVariantList TMultipartFormData::formItemVariantList(const QString &key) const
{
    return THttpRequest::itemVariantList(key, postParameters);
}

/*!
  Returns the map of variant value whose key is equal to \a key from
  the multipart/form-data.
 */
QVariantMap TMultipartFormData::formItems(const QString &key) const
{
    return THttpRequest::itemMap(key, postParameters);
}

/*!
  Returns a QVariantMap object with the form items of this
  multipart/form-data.
*/
QVariantMap TMultipartFormData::formItems() const
{
    return THttpRequest::itemMap(postParameters);
}

/*!
  Returns the value of the header field content-type in the MIME entity
  associated with the name \a dataName.
*/
QString TMultipartFormData::contentType(const QByteArray &dataName) const
{
    return entity(dataName).contentType();
}

/*!
  Returns the original name of the file contained in the MIME entity
  associated with the name \a dataName.
*/
QString TMultipartFormData::originalFileName(const QByteArray &dataName) const
{
    return entity(dataName).originalFileName();
}

/*!
  Returns the size of the file contained in the MIME entity
  associated with the name \a dataName.
  \warning Note that this method must be called before
  renameUploadedFile() method calls.
 */
qint64 TMultipartFormData::size(const QByteArray &dataName) const
{
    return entity(dataName).fileSize();
}

/*!
  Renames the file contained in the MIME entity associated with the
  name \a dataName.
  \warning Note that this method must not be called more than once.
  \sa TMimeEntity::renameUploadedFile()
 */
bool TMultipartFormData::renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite, QFile::Permissions permissions)
{
    return entity(dataName).renameUploadedFile(newName, overwrite, permissions);
}

/*!
  Returns the path of the temporary file contained in the MIME entity
  associated with the name \a dataName, including the absolute path.
 */
QString TMultipartFormData::uploadedFilePath(const QByteArray &dataName) const
{
    return entity(dataName).uploadedFilePath();
}

/*!
  Reads from the I/O device \a dev and parses it.
*/
void TMultipartFormData::parse(QIODevice *dev)
{
    if (!dev->isOpen()) {
        if (!dev->open(QIODevice::ReadOnly)) {
            return;
        }
    }

    while (!dev->atEnd()) {  // up to EOF
        TMimeHeader header = parseMimeHeader(dev);
        if (!header.isEmpty()) {
            QByteArray type = header.header("content-type");
            if (!type.isEmpty()) {
                if (!header.originalFileName().isEmpty()) {
                    QString contFile = writeContent(dev);
                    if (!contFile.isEmpty()) {
                        uploadedFiles << TMimeEntity(header, contFile);
                    }
                }
            } else {
                QByteArray name = header.dataName();
                QByteArray cont = parseContent(dev);

                QTextCodec *codec = Tf::app()->codecForHttpOutput();
                postParameters << QPair<QString, QString>(codec->toUnicode(name), codec->toUnicode(cont));
            }
        }
    }
}

/*!
  Reads MIME headers from the I/O device \a dev and parses it.
*/
TMimeHeader TMultipartFormData::parseMimeHeader(QIODevice *dev) const
{
    if (!dev->isOpen()) {
        return TMimeHeader();
    }

    TMimeHeader header;
    while (!dev->atEnd()) {
        QByteArray line = dev->readLine();
        if (line == CRLF || (!dataBoundary.isEmpty() && line.startsWith(dataBoundary))) {
            break;
        }

        int i = line.indexOf(':');
        if (i > 0) {
            header.setHeader(line.left(i).trimmed(), line.mid(i + 1).trimmed());
        }
    }
    return header;
}

/*!
  Reads MIME contents from the I/O device \a dev and parses it.
*/
QByteArray TMultipartFormData::parseContent(QIODevice *dev) const
{
    if (!dev->isOpen()) {
        return QByteArray();
    }

    QByteArray content;
    while (!dev->atEnd()) {
        QByteArray line = dev->readLine();
        if (line.startsWith(dataBoundary)) {
            break;
        }
        content += line;
    }
    return content.trimmed();
}

/*!
  Parses the multipart data and writes the one content to a file.
  Returns the file name.
 */
QString TMultipartFormData::writeContent(QIODevice *dev) const
{
    if (!dev->isOpen()) {
        return QString();
    }

    TTemporaryFile &out = Tf::currentContext()->createTemporaryFile();
    if (!out.open()) {
        return QString();
    }

    while (!dev->atEnd()) {
        QByteArray line = dev->readLine();
        if (line.startsWith(dataBoundary)) {
            qint64 size = qMax(out.size() - 2, Q_INT64_C(0));
            out.resize(size);  // Truncates the CR+LF
            break;
        }

        if (out.write(line) < 0) {
            return QString();
        }
    }
    out.close();
    return out.absoluteFilePath();
}

/*!
  Returns true if the MIME entity object associated with the name \a dataName
  exists; otherwise false.
*/
bool TMultipartFormData::hasEntity(const QByteArray &dataName) const
{
    for (auto &p : uploadedFiles) {
        if (p.header().dataName() == dataName) {
            return true;
        }
    }
    return false;
}

/*!
  Returns the MIME entity object associated with the name \a dataName.
*/
TMimeEntity TMultipartFormData::entity(const QByteArray &dataName) const
{
    for (auto &p : uploadedFiles) {
        if (p.header().dataName() == dataName) {
            return p;
        }
    }
    return TMimeEntity();
}

/*!
  Returns a list of the MIME entity objects associated with the name
  \a dataName.
*/
QList<TMimeEntity> TMultipartFormData::entityList(const QByteArray &dataName) const
{
    QList<TMimeEntity> list;

    QByteArray k = dataName;
    if (!k.endsWith("[]")) {
        k += QByteArrayLiteral("[]");
    }

    for (auto &p : uploadedFiles) {
        if (p.header().dataName() == k) {
            list << p;
        }
    }
    return list;
}


/*!
  \fn const TMimeHeader &TMimeEntity::header() const
  Returns a reference to the MIME header contained in this entity.
*/

/*!
  \fn TMimeHeader &TMimeEntity::header()
  Returns a reference to the MIME header contained in this entity.
*/

/*!
  \fn QByteArray TMimeEntity::header(const QByteArray &headerName) const
  Returns the value of the header \a headerName contained in this entity.
*/

/*!
 \fn QByteArray TMimeEntity::dataName() const
 Returns the parameter 'name' of the header field content-disposition
 in this entity.
*/

/*!
  \fn QString TMimeEntity::originalFileName() const
  Returns the original name of the file contained in this entity.
*/

/*!
  \fn bool TMimeHeader::isEmpty() const
  Returns true if the MIME header is empty; otherwise returns false.
*/

/*!
  \fn bool TMultipartFormData::hasFormItem(const QString &name) const
  Returns true if there is a string pair whose name is equal to
  \a name from the multipart/form-data; otherwise returns false.
*/

/*!
  \fn QString TMultipartFormData::formItemValue(const QString &name) const
  Returns the first form string value whose name is equal to \a name
  from the multipart/form-data.
*/
