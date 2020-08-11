#pragma once
#include <QString>
#include <TGlobal>
#include <TInternetMessageHeader>

class QTextCodec;


class T_CORE_EXPORT TMailMessage : public TInternetMessageHeader {
public:
    TMailMessage(const QByteArray &encoding = "UTF-8");
    TMailMessage(const char *encoding);
    TMailMessage(const QString &str, const QByteArray &encoding = "UTF-8");
    TMailMessage(const TMailMessage &other);

    QString subject() const;
    void setSubject(const QString &subject);
    QByteArray from() const;
    QByteArray fromAddress() const;
    void setFrom(const QByteArray &address, const QString &friendlyName = QString());
    QByteArray to() const;
    void addTo(const QByteArray &address, const QString &friendlyName = QString());
    QByteArray cc() const;
    void addCc(const QByteArray &address, const QString &friendlyName = QString());
    QByteArray bcc() const;
    void addBcc(const QByteArray &address, const QString &friendlyName = QString());
    QString body() const;
    void setBody(const QString &body);
    QByteArray toByteArray() const;
    QByteArrayList recipients() const { return recipientList; }
    TMailMessage &operator=(const TMailMessage &other);

protected:
    void parse(const QString &str);
    QByteArrayList addresses(const QByteArray &field) const;
    void addAddress(const QByteArray &field, const QByteArray &address, const QString &friendlyName);
    void addRecipient(const QByteArray &address);
    void addRecipients(const QByteArrayList &addresses);

private:
    void init(const QByteArray &encoding);

    QByteArray mailBody;
    QTextCodec *textCodec {nullptr};
    QByteArrayList recipientList;
};

