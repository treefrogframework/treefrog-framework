#pragma once
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <TGlobal>


class T_CORE_EXPORT TOAuth2Client {
public:
    enum ErrorCode {
        NoError = 0,  ///< no error
        InvalidRequest,  ///< invalid request
        InvalidClient,  ///< invalid client
        InvalidGrant,  ///< invalid grant
        UnauthorizedClient,  ///< unauthorized client
        UnsupportedGrantType,  ///< unsupported grant type
        AccessDenied,  ///< access denied
        UnsupportedResponseType,  ///< unsupported response type
        InvalidScope,  ///< invalid scope
        ServerError,  ///< server error
        TemporarilyUnavailable,  ///< temporarily unavailable
        UnknownError,  ///< unknown error
    };

    TOAuth2Client(const QString &clientId, const QString &clientSecret = QString());
    TOAuth2Client(const TOAuth2Client &other) = default;
    TOAuth2Client &operator=(const TOAuth2Client &other) = default;

    QUrl startAuthorization(const QUrl &requestUrl, const QStringList &scopes, const QString &state, const QUrl &redirect, const QVariantMap &parameters = QVariantMap(), int msecs = 5000);
    QString requestAccessToken(const QUrl &requestUrl, const QString &code, const QVariantMap &parameters = QVariantMap(), int msecs = 5000);
    ErrorCode errorCode() { return _errorCode; }
    QNetworkReply::NetworkError networkError() const { return _networkError; }

private:
    QString _clientId;
    QString _clientSecret;
    ErrorCode _errorCode {NoError};
    QNetworkReply::NetworkError _networkError {QNetworkReply::NoError};
};

