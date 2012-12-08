#ifndef TFEXCEPTION_H
#define TFEXCEPTION_H

#include <QtCore/qtconcurrentexception.h>
#include <TGlobal>


class T_CORE_EXPORT RuntimeException : public QtConcurrent::Exception
{
public:
    RuntimeException(const RuntimeException &e)
        : Exception(e), msg(e.msg), file(e.file), line(e.line) { }
    RuntimeException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~RuntimeException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual Exception *clone() const { return new RuntimeException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT SecurityException : public QtConcurrent::Exception
{
public:
    SecurityException(const SecurityException &e)
        : Exception(e), msg(e.msg), file(e.file), line(e.line) { }
    SecurityException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~SecurityException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual Exception *clone() const { return new SecurityException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT SqlException : public QtConcurrent::Exception
{
public:
    SqlException(const SqlException &e)
        : Exception(e), msg(e.msg), file(e.file), line(e.line) { }
    SqlException(const QString &message, const char *fileName = "", int lineNumber = 0)
        : msg(message), file(fileName), line(lineNumber) { }
    virtual ~SqlException() throw() { }
    QString message() const { return msg; }
    QString fileName() const { return file; }
    int lineNumber() const { return line; }
    virtual void raise() const { throw *this; }
    virtual Exception *clone() const { return new SqlException(*this); }

private:
    QString msg;
    QString file;
    int line;
};


class T_CORE_EXPORT ClientErrorException : public QtConcurrent::Exception
{
public:
    ClientErrorException(const ClientErrorException &e)
        : Exception(e), code(e.code) { }
    ClientErrorException(int statusCode)
        : code(statusCode) { }
    virtual ~ClientErrorException() throw() { }
    int statusCode() const { return code; }
    virtual void raise() const { throw *this; }
    virtual Exception *clone() const { return new ClientErrorException(*this); }

private:
    int code;
};

#endif // TFEXCEPTION_H
