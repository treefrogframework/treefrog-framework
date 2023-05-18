#pragma once
#include <QFile>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <TGlobal>

class TActionContext;
class QIODevice;


class T_CORE_EXPORT TMimeHeader {
public:
    TMimeHeader() { }
    TMimeHeader(const TMimeHeader &other);
    TMimeHeader &operator=(const TMimeHeader &other);

    bool isEmpty() const { return _headers.isEmpty(); }
    QByteArray header(const QByteArray &headerName) const;
    void setHeader(const QByteArray &headerName, const QByteArray &value);
    QByteArray contentDispositionParameter(const QByteArray &name) const;
    QByteArray dataName() const;
    QString originalFileName() const;

protected:
    static QMap<QByteArray, QByteArray> parseHeaderParameter(const QByteArray &header);

private:
    static int skipWhitespace(const QByteArray &text, int pos);
    QList<QPair<QByteArray, QByteArray>> _headers;
};


class T_CORE_EXPORT TMimeEntity {
public:
    static const QFile::Permissions DefaultPermissions;

    TMimeEntity() { }
    TMimeEntity(const TMimeEntity &other);
    TMimeEntity &operator=(const TMimeEntity &other);

    const TMimeHeader &header() const { return _entity.first; }
    TMimeHeader &header() { return _entity.first; }
    QByteArray header(const QByteArray &headerName) const { return _entity.first.header(headerName); }
    QByteArray dataName() const { return _entity.first.dataName(); }
    QString contentType() const;
    int64_t fileSize() const;
    QString originalFileName() const { return _entity.first.originalFileName(); }
    bool renameUploadedFile(const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    QString uploadedFilePath() const;

private:
    TMimeEntity(const TMimeHeader &header, const QString &body);
    QPair<TMimeHeader, QString> _entity;
    friend class TMultipartFormData;
};


class T_CORE_EXPORT TMultipartFormData {
public:
    static const QFile::Permissions DefaultPermissions;

    TMultipartFormData(const QByteArray &boundary = QByteArray());
    TMultipartFormData(const QByteArray &formData, const QByteArray &boundary, TActionContext *context);
    TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary, TActionContext *context);
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
    int64_t size(const QByteArray &dataName) const;
    bool renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite = false, QFile::Permissions permissions = DefaultPermissions);
    QString uploadedFilePath(const QByteArray &dataName) const;
    void clear();

    bool hasEntity(const QByteArray &dataName) const;
    TMimeEntity entity(const QByteArray &dataName) const;
    QList<TMimeEntity> entityList(const QByteArray &dataName) const;

protected:
    void parse(QIODevice *dev, TActionContext *context);

private:
    TMimeHeader parseMimeHeader(QIODevice *dev) const;
    QByteArray parseContent(QIODevice *dev) const;
    QString writeContent(QIODevice *dev, TActionContext *context) const;

    QByteArray dataBoundary;
    QList<QPair<QString, QString>> postParameters;
    QList<TMimeEntity> uploadedFiles;
    QString bodyFile;

    friend class THttpRequest;
};


Q_DECLARE_METATYPE(TMimeHeader)
Q_DECLARE_METATYPE(TMimeEntity)
Q_DECLARE_METATYPE(TMultipartFormData)

