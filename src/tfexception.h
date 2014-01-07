#ifndef TFEXCEPTION_H
#define TFEXCEPTION_H

#include <exception>
#include <QString>
#include <TGlobal>


class T_CORE_EXPORT RuntimeException : public std::exception
{
public:
    RuntimeException(const RuntimeException &e)
        : std::exception(e), msg(e.msg), file(e.file), line(e.line) { }
    RuntimeException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~RuntimeException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new RuntimeException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT SecurityException : public std::exception
{
public:
    SecurityException(const SecurityException &e)
        : std::exception(e), msg(e.msg), file(e.file), line(e.line) { }
    SecurityException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~SecurityException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new SecurityException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT SqlException : public std::exception
{
public:
    SqlException(const SqlException &e)
        : std::exception(e), msg(e.msg), file(e.file), line(e.line) { }
    SqlException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~SqlException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new SqlException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT KvsException : public std::exception
{
public:
    KvsException(const KvsException &e)
        : std::exception(e), msg(e.msg), file(e.file), line(e.line) { }
    KvsException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~KvsException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new KvsException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT ClientErrorException : public std::exception
{
public:
    ClientErrorException(const ClientErrorException &e)
        : std::exception(e), code(e.code) { }
    ClientErrorException(int statusCode)
        : code(statusCode) { }
    virtual ~ClientErrorException() throw() { }
    int statusCode() const { return code; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new ClientErrorException(*this); }

private:
    int code;
};


class T_CORE_EXPORT StandardException : public std::exception
{
public:
    StandardException(const StandardException &e)
        : std::exception(e), msg(e.msg), file(e.file), line(e.line) { }
    StandardException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~StandardException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual std::exception *clone() const { return new StandardException(*this); }

private:
    QString msg;
    QString file;
    int line;
};

#endif // TFEXCEPTION_H
