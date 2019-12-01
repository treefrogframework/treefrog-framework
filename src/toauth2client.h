#ifndef TOAUTH2CLIENT_H
#define TOAUTH2CLIENT_H

#include <TGlobal>
#include <QStringList>
#include <QUrl>
#include <QNetworkReply>
#include <QVariantMap>


class T_CORE_EXPORT TOAuth2Client
{
public:
    enum ErrorCode {
        NoError = 0,      ///< no error
        InvalidRequest,   ///< invalid request
        InvalidClient,    ///< invalid client
        InvalidGrant,     ///< invalid grant
        UnauthorizedClient,      ///< unauthorized_client
        UnsupportedGrantType,    ///< unsupported grant type
        AccessDenied,            ///< access_denied
        UnsupportedResponseType, ///<unsupported_response_type
        InvalidScope,            ///< invalid scope
        ServerError,             ///< server_error
        TemporarilyUnavailable,  ///< temporarily_unavailable
        UnknownError,            ///< unknown error
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

#endif // TOAUTH2CLIENT_H
