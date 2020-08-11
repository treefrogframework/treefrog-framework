#pragma once
#include <TActionView>


class T_CORE_EXPORT TTextView : public TActionView {
    Q_OBJECT
public:
    TTextView(const QString &text = QString());
    virtual ~TTextView() { }

    void setText(const QString &text);
    QString toString();

private:
    QString viewText;

    T_DISABLE_COPY(TTextView)
    T_DISABLE_MOVE(TTextView)
};


inline TTextView::TTextView(const QString &text) :
    TActionView(), viewText(text)
{
}

inline void TTextView::setText(const QString &text)
{
    viewText = text;
}

inline QString TTextView::toString()
{
    return viewText;
}

