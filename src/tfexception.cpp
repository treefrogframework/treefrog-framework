#include "tfexception.h"

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
  \fn QString RuntimeException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString RuntimeException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int RuntimeException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void RuntimeException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *RuntimeException::clone() const
  \brief Creates and returns a deep copy of the current data.
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
  \fn QString SecurityException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString SecurityException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int SecurityException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void SecurityException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *SecurityException::clone() const
  \brief Creates and returns a deep copy of the current data.
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
  \fn QString SqlException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString SqlException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int SqlException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void SqlException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *SqlException::clone() const
  \brief Creates and returns a deep copy of the current data.
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
  \fn ClientErrorException::ClientErrorException(int statusCode)
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
  \fn virtual void ClientErrorException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *ClientErrorException::clone() const
  \brief Creates and returns a deep copy of the current data.
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
  \fn QString KvsException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString KvsException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int KvsException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void KvsException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *KvsException::clone() const
  \brief Creates and returns a deep copy of the current data.
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
  \fn QString StandardException::message() const
  \brief Returns the message.
*/

/*!
  \fn QString StandardException::fileName() const
  \brief Returns the file name.
*/

/*!
  \fn int StandardException::lineNumber() const
  \brief Return the line number.
*/

/*!
  \fn virtual void StandardException::raise() const
  \brief Raises the exception.
*/

/*!
  \fn virtual Exception *StandardException::clone() const
  \brief Creates and returns a deep copy of the current data.
*/
