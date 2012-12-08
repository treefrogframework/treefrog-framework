/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QTextCodec>
#include <TWebApplication>
#include <TMultipartFormData>
#include <THttpUtility>
#include <TActionContext>
#include <TTemporaryFile>

/*!
  \class TMimeHeader
  \brief The TMimeHeader class contains MIME header information for internet.
*/

/*!
  Copy constructor.
*/
TMimeHeader::TMimeHeader(const TMimeHeader &other)
    : headers(other.headers)
{ }


QByteArray TMimeHeader::header(const QByteArray &headerName) const
{
    QByteArray name = headerName.toLower();
    for (QListIterator<QPair<QByteArray, QByteArray> > i(headers); i.hasNext(); ) {
        const QPair<QByteArray, QByteArray> &p = i.next();
        if (p.first.toLower() == name) {
            return p.second;
        }
    }
    return QByteArray();
}


void TMimeHeader::setHeader(const QByteArray &headerName, const QByteArray &value)
{
    headers << qMakePair(headerName, value);
}


QByteArray TMimeHeader::contentDispositionParameter(const QByteArray &name) const
{
    QByteArray disp = header("content-disposition");
    QHash<QByteArray, QByteArray> params = parseHeaderParameter(disp);
    return params[name];
}


QByteArray TMimeHeader::dataName() const
{
    return contentDispositionParameter("name");
}


QByteArray TMimeHeader::originalFileName() const
{
    return contentDispositionParameter("filename");
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


QHash<QByteArray, QByteArray> TMimeHeader::parseHeaderParameter(const QByteArray &header)
{
    QHash<QByteArray, QByteArray> result;
    int pos = 0;
    
    for (;;) {
        pos = skipWhitespace(header, pos);
        if (pos >= header.length())
            return result;

        int semicol = header.indexOf(';', pos);
        if (semicol < 0) 
            semicol = header.length();

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

        key = header.mid(pos,  equal - pos).trimmed();
        pos = equal + 1;        
        
        pos = skipWhitespace(header, pos);
        if (pos >= header.length())
            return result;

        QByteArray value;
        if (header[pos] == '"') {
            ++pos;
            while (pos < header.length()) {
                char c = header.at(pos);
                if (c == '"') {
                    // end of quoted text
                    break;
                } else if (c == '\\') {
                    ++pos;
                    if (pos >= header.length()) {
                        // broken header
                        return result;
                    }
                    c = header[pos];
                }
                
                value += c;
                ++pos;
            }
        } else {
            while (pos < header.length()) {
                char c = header.at(pos);
                if (c == ' ' || c == '\t' || c == '\r'
                    || c == '\n' || c == ';') {
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

TMimeEntity::TMimeEntity(const TMimeEntity &other)
    : QPair<TMimeHeader, QString>(other.first, other.second)
{ }


TMimeEntity::TMimeEntity(const TMimeHeader &header, const QString &body)
{   
    first = header;
    second = body;
}


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
    QFileInfo fi(second);
    if (!fi.exists()) {
        return -1;
    }
    return fi.size();
}


bool TMimeEntity::renameUploadedFile(const QString &newName, bool overwrite)
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
    return file.rename(newpath);
}


QString TMimeEntity::uploadedFilePath() const
{
    // check original filename
    return (header().isEmpty() || header().contentDispositionParameter("filename").isEmpty()) ? QString() : second;
}


/*!
  \class TMultipartFormData
  \brief The TMultipartFormData represents a media-type multipart/form-data.
*/

TMultipartFormData::TMultipartFormData(const QByteArray &boundary)
    : dataBoundary(boundary)
{ }


TMultipartFormData::TMultipartFormData(const QByteArray &formData, const QByteArray &boundary)
    : dataBoundary(boundary)
{
    QByteArray data(formData);
    QBuffer buffer(&data);
    parse(&buffer);
}


TMultipartFormData::TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary)
    : dataBoundary(boundary)
{
    QFile file(bodyFilePath);
    parse(&file);
}


bool TMultipartFormData::isEmpty() const
{
    return postParameters.isEmpty() && uploadedFiles.isEmpty();
}


QStringList TMultipartFormData::allFormItemValues(const QString &name) const
{
    QStringList ret;
    QVariantList values = postParameters.values(name);
    for (QListIterator<QVariant> it(values); it.hasNext(); ) {
        ret << it.next().toString();
    }
    return ret;
}


QString TMultipartFormData::contentType(const QByteArray &dataName) const
{
    return entity(dataName).contentType();
}


QString TMultipartFormData::originalFileName(const QByteArray &dataName) const
{
    return entity(dataName).originalFileName();
}


/*!
    \warning Note that this method must be called before renameUploadedFile() method calls.
 */
qint64 TMultipartFormData::size(const QByteArray &dataName) const
{
    return entity(dataName).fileSize();
}


/*!
    \warning Note that this method must not be called more than once.
 */
bool TMultipartFormData::renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite)
{
    return entity(dataName).renameUploadedFile(newName, overwrite);
}


void TMultipartFormData::parse(QIODevice *data)
{
    if (!data->isOpen()) {
        if (!data->open(QIODevice::ReadOnly)) {
            return;
        }
    }

    while (!data->atEnd()) {  // up to EOF
        TMimeHeader header = parseMimeHeader(data);
        if (!header.isEmpty()) {
            QByteArray type = header.header("content-type");
            if (!type.isEmpty()) {
                if (!header.originalFileName().isEmpty()) {
                    QString contFile = writeContent(data);
                    if (!contFile.isEmpty()) {
                        uploadedFiles << TMimeEntity(header, contFile);
                    }
                }
            } else {
                QByteArray name = header.dataName();
                QByteArray cont = parseContent(data);

                QTextCodec *codec = Tf::app()->codecForHttpOutput();
                postParameters.insertMulti(codec->toUnicode(name), codec->toUnicode(cont));
            }
        }
    }
}


TMimeHeader TMultipartFormData::parseMimeHeader(QIODevice *data) const
{
    if (!data->isOpen()) {
        return TMimeHeader();
    }

    TMimeHeader header;
    while (!data->atEnd()) {
        QByteArray line = data->readLine();
        if (line == "\r\n" || line.startsWith(dataBoundary)) {
            break;
        }
    
        int i = line.indexOf(':');
        if (i > 0) {
            header.setHeader(line.left(i).trimmed(), line.mid(i + 1).trimmed());
        }
    }
    return header;
}


QByteArray TMultipartFormData::parseContent(QIODevice *data) const
{
    if (!data->isOpen()) {
        return QByteArray();
    }

    QByteArray content;
    while (!data->atEnd()) {
        QByteArray line = data->readLine();
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
QString TMultipartFormData::writeContent(QIODevice *data) const
{
    if (!data->isOpen()) {
        return QString();
    }

    TTemporaryFile &out = TActionContext::current()->createTemporaryFile();
    if (!out.open()) {
        return QString();
    }

    while (!data->atEnd()) {
        QByteArray line = data->readLine();
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


TMimeEntity TMultipartFormData::entity(const QByteArray &dataName) const
{
    for (QListIterator<TMimeEntity> i(uploadedFiles); i.hasNext(); ) {
        const TMimeEntity &p = i.next();
        if (p.header().dataName() == dataName) {
            return p;
        }
    }
    return TMimeEntity();
}


QList<TMimeEntity> TMultipartFormData::entityList(const QByteArray &dataName) const
{
    QList<TMimeEntity> list;

    QByteArray k = dataName;
    if (!k.endsWith("[]")) {
        k += QLatin1String("[]");
    }
    
    for (QListIterator<TMimeEntity> i(uploadedFiles); i.hasNext(); ) {
        const TMimeEntity &p = i.next();
        if (p.header().dataName() == k) {
            list << p;
        }
    }
    return list;
}
