#include "tfexception.h"

/*!
  \class TfException
  \brief The TfException class is a base class for all TreeFrog exception classes.
*/

/*!
  \fn TfException::TfException(const TfException &e)
  \brief Copy constructor.
*/

/*!
  \fn TfException::TfException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual TfException::~TfException() throw()
  \brief Destructor.
*/

/*!
  \fn QString TfException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString TfException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int TfException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void TfException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *TfException::clone() const
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn virtual QString TfException::className() const
  \brief Returns exception class name.
*/


/*!
  \class RuntimeException
  \brief The RuntimeException class represents an exception that
  can be thrown when runtime error occurs.
*/

/*!
  \fn RuntimeException::RuntimeException(const RuntimeException &e)
  \brief Copy constructor.
*/

/*!
  \fn RuntimeException::RuntimeException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual RuntimeException::~RuntimeException() throw()
  \brief Destructor.
*/

/*!
  \fn void RuntimeException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *RuntimeException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString RuntimeException::className() const override
  \brief Returns exception class name.
*/


/*!
  \class SecurityException
  \brief The SecurityException class represents an exception that
  can be thrown when a security issue is detected.
*/

/*!
  \fn SecurityException::SecurityException(const SecurityException &e)
  \brief Copy constructor.
*/

/*!
  \fn SecurityException::SecurityException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual SecurityException::~SecurityException() throw()
  \brief Destructor.
*/

/*!
  \fn void SecurityException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *SecurityException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString SecurityException::className() const override
  \brief Returns exception class name.
*/


/*!
  \class SqlException
  \brief The SqlException class represents an exception that
  can be thrown when SQL database error occurs.
*/

/*!
  \fn SqlException::SqlException(const SqlException &e)
  \brief Copy constructor.
*/

/*!
  \fn SqlException::SqlException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual SqlException::~SqlException() throw()
  \brief Destructor.
*/

/*!
  \fn void SqlException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *SqlException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString SqlException::className() const override
  \brief Returns exception class name.
*/


/*!
  \class ClientErrorException
  \brief The ClientErrorException class represents an exception
  that can be thrown when communication error with a HTTP client
  occurs.
*/

/*!
  \fn ClientErrorException::ClientErrorException(const ClientErrorException &e)
  \brief Copy constructor.
*/

/*!
  \fn ClientErrorException::ClientErrorException(int statusCode, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual ClientErrorException::~ClientErrorException() throw()
  \brief Destructor.
*/

/*!
  \fn int ClientErrorException::statusCode() const
  \brief Returns the status code.
*/

/*!
  \fn void ClientErrorException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *ClientErrorException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString ClientErrorException::className() const override
  \brief Returns exception class name.
*/


/*!
  \class KvsException
  \brief The KvsException class represents an exception that
  can be thrown when KVS database error occurs.
*/

/*!
  \fn KvsException::KvsException(const KvsException &e)
  \brief Copy constructor.
*/

/*!
  \fn KvsException::KvsException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual KvsException::~KvsException() throw()
  \brief Destructor.
*/

/*!
  \fn void KvsException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *KvsException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString KvsException::className() const override
  \brief Returns exception class name.
*/


/*!
  \class StandardException
  \brief The StandardException class represents an exception that
  can be thrown when standard error occurs in source code of your
  web application.
*/

/*!
  \fn StandardException::StandardException(const StandardException &e)
  \brief Copy constructor.
*/

/*!
  \fn StandardException::StandardException(const QString &message, const char *fileName, int lineNumber)
  \brief Constructor.
*/

/*!
  \fn virtual StandardException::~StandardException() throw()
  \brief Destructor.
*/

/*!
  \fn void StandardException::raise() const override
  \brief Raises the exception.
*/

/*!
  \fn Exception *StandardException::clone() const override
  \brief Creates and returns a deep copy of the current data.
*/

/*!
  \fn QString StandardException::className() const override
  \brief Returns exception class name.
*/
