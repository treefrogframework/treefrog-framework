#pragma once
#include <QByteArray>
#include <QString>
#include <TGlobal>
#include <exception>


class T_CORE_EXPORT TfException : public std::exception {
public:
    TfException(const QString &message, const char *fileName = "", int lineNumber = 0) noexcept :
        msg(message),
        file(fileName), line(lineNumber)
    {
        whatmsg = message.toLocal8Bit();
        if (lineNumber > 0) {
            whatmsg += " [";
            whatmsg += fileName;
            whatmsg += ":" + QByteArray::number(lineNumber) + "]";
        }
    }
    TfException(const TfException &e) noexcept :
        std::exception(e),
        msg(e.msg), file(e.file), line(e.line), whatmsg(e.whatmsg) { }
    virtual ~TfException() throw() { }

    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }

    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new TfException(*this); }
    virtual QString className() const { return QStringLiteral("TfException"); }
    virtual const char *what() const noexcept override { return whatmsg.constData(); }

protected:
    QString msg;
    QString file;
    int line {0};
    QByteArray whatmsg;
};


class T_CORE_EXPORT RuntimeException : public TfException {
public:
    RuntimeException(const QString &message, const char *fileName = "", int lineNumber = 0) :
        TfException(message, fileName, lineNumber) { }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new RuntimeException(*this); }
    QString className() const override { return QStringLiteral("RuntimeException"); }
};


class T_CORE_EXPORT SecurityException : public TfException {
public:
    SecurityException(const QString &message, const char *fileName = "", int lineNumber = 0) :
        TfException(message, fileName, lineNumber) { }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new SecurityException(*this); }
    QString className() const override { return QStringLiteral("SecurityException"); }
};


class T_CORE_EXPORT SqlException : public TfException {
public:
    SqlException(const QString &message, const char *fileName = "", int lineNumber = 0) :
        TfException(message, fileName, lineNumber) { }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new SqlException(*this); }
    QString className() const override { return QStringLiteral("SqlException"); }
};


class T_CORE_EXPORT KvsException : public TfException {
public:
    KvsException(const QString &message, const char *fileName = "", int lineNumber = 0) :
        TfException(message, fileName, lineNumber) { }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new KvsException(*this); }
    QString className() const override { return QStringLiteral("KvsException"); }
};


class T_CORE_EXPORT ClientErrorException : public TfException {
public:
    ClientErrorException(int statusCode, const char *fileName = "", int lineNumber = 0) :
        TfException(QStringLiteral("HTTP status code: %1").arg(statusCode), fileName, lineNumber),
        code(statusCode) { }

    int statusCode() const { return code; }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new ClientErrorException(*this); }
    QString className() const override { return QStringLiteral("ClientErrorException"); }

private:
    int code;
};


class T_CORE_EXPORT StandardException : public TfException {
public:
    StandardException(const QString &message, const char *fileName = "", int lineNumber = 0) :
        TfException(message, fileName, lineNumber) { }

    void raise() const override { throw *this; }
    std::exception *clone() const override { return new StandardException(*this); }
    QString className() const override { return QStringLiteral("StandardException"); }
};

