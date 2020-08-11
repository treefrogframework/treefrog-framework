#pragma once
#include <TActionView>
#include <TSession>


class T_CORE_EXPORT TDirectView : public TActionView {
public:
    TDirectView() :
        TActionView() { }
    virtual ~TDirectView() { }

protected:
    const TSession &session() const;
    TSession &session();
    bool addCookie(const TCookie &cookie);
    bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false);

private:
    T_DISABLE_COPY(TDirectView)
    T_DISABLE_MOVE(TDirectView)
};

