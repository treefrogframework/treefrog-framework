#ifndef TSESSION_H
#define TSESSION_H

#include <QVariant>
#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TSession : public QVariantHash
{
public:
    TSession(const QByteArray &id = QByteArray());
    TSession(const TSession &);
    TSession &operator=(const TSession &);

    QByteArray id() const { return sessionId; }
    void reset();

    static QByteArray sessionName();

private:
    QByteArray sessionId;

    void clear() {} // disabled
    friend class TSessionCookieStore;
    friend class TActionContext;
};


inline TSession::TSession(const QByteArray &id)
    : sessionId(id)
{ }

inline TSession::TSession(const TSession &session)
    : QVariantHash(*static_cast<const QVariantHash *>(&session)), sessionId(session.sessionId)
{ }

inline TSession &TSession::operator=(const TSession &session)
{
    QVariantHash::operator=(*static_cast<const QVariantHash *>(&session));
    sessionId = session.sessionId;
    return *this;
}

#endif // TSESSION_H
