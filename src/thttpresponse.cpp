/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QBuffer>
#include <QFile>
#include <THttpResponse>
#include <THttpUtility>
#include <TSystemGlobal>

/*!
  \class THttpResponse
  \brief The THttpResponse class contains response information for HTTP.
*/

/*!
  \fn THttpResponse::THttpResponse()
  Constructor.
*/

/*!
  Constructor with the header \a header and the body \a body.
*/
THttpResponse::THttpResponse(const THttpResponseHeader &header, const QByteArray &body) :
    resHeader(header), tmpByteArray(body)
{
    if (!tmpByteArray.isNull()) {
        bodyDevice = new QBuffer(&tmpByteArray);
    }
}

/*!
  Destructor.
*/
THttpResponse::~THttpResponse()
{
    delete bodyDevice;
}

/*!
  Returns true if the body is null; otherwise returns false.
*/
bool THttpResponse::isBodyNull() const
{
    return !bodyDevice;
}

/*!
  Sets the body to \a body.
 */
void THttpResponse::setBody(const QByteArray &body)
{
    delete bodyDevice;
    tmpByteArray = body;
    bodyDevice = (tmpByteArray.isNull()) ? nullptr : new QBuffer(&tmpByteArray);
}

/*!
  Returns the body.
 */
QByteArray THttpResponse::body() const
{
    if (bodyDevice) {
        QBuffer *buf = dynamic_cast<QBuffer *>(bodyDevice);
        if (buf) {
            return buf->buffer();
        }
    }
    return QByteArray();
}

/*!
  Sets the file to read the content from the given \a filePath.
*/
void THttpResponse::setBodyFile(const QString &filePath)
{
    delete bodyDevice;
    bodyDevice = nullptr;

    QFile *fp = new QFile(filePath);
    if (fp->exists()) {
        if (fp->open(QIODevice::ReadOnly)) {
            // Success!
            bodyDevice = fp;
            return;
        } else {
            tSystemError("faild to open file: %s", qUtf8Printable(filePath));
        }
    } else {
        tSystemError("file not found: %s", qUtf8Printable(filePath));
    }

    // Error
    delete fp;
}


/*!
  \fn THttpResponseHeader &THttpResponse::header()
  Return the HTTP header.
*/

/*!
  \fn const THttpResponseHeader &THttpResponse::header() const
  Return the HTTP header.
*/

/*!
  \fn QIODevice *THttpResponse::bodyIODevice()
  Returns the IO device of the body currently set.
*/

/*!
  \fn qint64 THttpResponse::bodyLength() const
  Returns the number of bytes of the body.
*/
