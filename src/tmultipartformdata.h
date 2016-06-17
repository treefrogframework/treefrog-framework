#ifndef TMULTIPARTFORMDATA_H
#define TMULTIPARTFORMDATA_H

#include <QStringList>
#include <QMap>
#include <QPair>
#include <QFile>
#include <TGlobal>

class QIODevice;


class T_CORE_EXPORT TMimeHeader
{
public:
    TMimeHeader() { }
    TMimeHeader(const TMimeHeader &other);
    bool isEmpty() const { return headers.isEmpty(); }
    QByteArray header(const QByteArray &headerName) const;
    void setHeader(const QByteArray &headerName, const QByteArray &value);
    QByteArray contentDispositionParameter(const QByteArray &name) const;
    QByteArray dataName() const;
    QString originalFileName() const;

protected:
    static QMap<QByteArray, QByteArray> parseHeaderParameter(const QByteArray &header);

private:
    static int skipWhitespace(const QByteArray &text, int pos);
    QList<QPair<QByteArray, QByteArray>>  headers;
};


class T_CORE_EXPORT TMimeEntity : protected QPair<TMimeHeader, QString>
{
public:
    static const QFile::Permissions DefaultPermissions;

    TMimeEntity() { }
    TMimeEntity(const TMimeEntity &other);

    const TMimeHeader &header() const { return first; }
    TMimeHeader &header() { return first; }
    QByteArray header(const QByteArray &headerName) const { return first.header(headerName); }
    QByteArray dataName() const { return first.dataName(); }
    QString contentType() const;
    qint64 fileSize() const;
    QString originalFileName() const { return first.originalFileName(); }
    bool renameUploadedFile(const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    QString uploadedFilePath() const;

private:
    TMimeEntity(const TMimeHeader &header, const QString &body);

    friend class TMultipartFormData;
};


class T_CORE_EXPORT TMultipartFormData
{
public:
    static const QFile::Permissions DefaultPermissions;

    TMultipartFormData(const QByteArray &boundary = QByteArray());
    TMultipartFormData(const QByteArray &formData, const QByteArray &boundary);
    TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary);
    ~TMultipartFormData() { }

    bool isEmpty() const;
    bool hasFormItem(const QString &name) const { return postParameters.contains(name); }
    QString formItemValue(const QString &name) const { return postParameters.value(name).toString(); }
    QStringList allFormItemValues(const QString &name) const;
    QVariantMap formItems(const QString &key) const;
    const QVariantMap &formItems() const { return postParameters; }

    QString contentType(const QByteArray &dataName) const;
    QString originalFileName(const QByteArray &dataName) const;
    qint64 size(const QByteArray &dataName) const;
    bool renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    void clear();

    TMimeEntity entity(const QByteArray &dataName) const;
    QList<TMimeEntity> entityList(const QByteArray &dataName) const;

protected:
    void parse(QIODevice *dev);

private:
    TMimeHeader parseMimeHeader(QIODevice *dev) const;
    QByteArray parseContent(QIODevice *dev) const;
    QString writeContent(QIODevice *dev) const;

    QByteArray dataBoundary;
    QVariantMap postParameters;
    QList<TMimeEntity> uploadedFiles;
};


Q_DECLARE_METATYPE(TMimeHeader)
Q_DECLARE_METATYPE(TMimeEntity)
Q_DECLARE_METATYPE(TMultipartFormData)

#endif // TMULTIPARTFORMDATA_H
