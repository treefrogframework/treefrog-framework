#pragma once
#include <QFile>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <TGlobal>

class QIODevice;


class T_CORE_EXPORT TMimeHeader {
public:
    TMimeHeader() { }
    TMimeHeader(const TMimeHeader &other);
    TMimeHeader &operator=(const TMimeHeader &other);

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
    QList<QPair<QByteArray, QByteArray>> headers;
};


class T_CORE_EXPORT TMimeEntity {
public:
    static const QFile::Permissions DefaultPermissions;

    TMimeEntity() { }
    TMimeEntity(const TMimeEntity &other);
    TMimeEntity &operator=(const TMimeEntity &other);

    const TMimeHeader &header() const { return entity.first; }
    TMimeHeader &header() { return entity.first; }
    QByteArray header(const QByteArray &headerName) const { return entity.first.header(headerName); }
    QByteArray dataName() const { return entity.first.dataName(); }
    QString contentType() const;
    qint64 fileSize() const;
    QString originalFileName() const { return entity.first.originalFileName(); }
    bool renameUploadedFile(const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    QString uploadedFilePath() const;

private:
    TMimeEntity(const TMimeHeader &header, const QString &body);
    QPair<TMimeHeader, QString> entity;
    friend class TMultipartFormData;
};


class T_CORE_EXPORT TMultipartFormData {
public:
    static const QFile::Permissions DefaultPermissions;

    TMultipartFormData(const QByteArray &boundary = QByteArray());
    TMultipartFormData(const QByteArray &formData, const QByteArray &boundary);
    TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary);
    ~TMultipartFormData() { }

    bool isEmpty() const;
    bool hasFormItem(const QString &name) const;
    QString formItemValue(const QString &name) const;
    QStringList allFormItemValues(const QString &name) const;
    QVariantList formItemVariantList(const QString &key) const;
    QVariantMap formItems(const QString &key) const;
    QVariantMap formItems() const;

    QString contentType(const QByteArray &dataName) const;
    QString originalFileName(const QByteArray &dataName) const;
    qint64 size(const QByteArray &dataName) const;
    bool renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    QString uploadedFilePath(const QByteArray &dataName) const;
    void clear();

    bool hasEntity(const QByteArray &dataName) const;
    TMimeEntity entity(const QByteArray &dataName) const;
    QList<TMimeEntity> entityList(const QByteArray &dataName) const;

protected:
    void parse(QIODevice *dev);

private:
    TMimeHeader parseMimeHeader(QIODevice *dev) const;
    QByteArray parseContent(QIODevice *dev) const;
    QString writeContent(QIODevice *dev) const;

    QByteArray dataBoundary;
    QList<QPair<QString, QString>> postParameters;
    QList<TMimeEntity> uploadedFiles;
    QString bodyFile;

    friend class THttpRequest;
};


Q_DECLARE_METATYPE(TMimeHeader)
Q_DECLARE_METATYPE(TMimeEntity)
Q_DECLARE_METATYPE(TMultipartFormData)

