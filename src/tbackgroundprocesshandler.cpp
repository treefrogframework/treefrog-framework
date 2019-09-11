#include "TBackgroundProcessHandler"
#include "TWebApplication"

/*!
  \class TBackgroundProcessHandler
  \brief The TBackgroundProcessHandler class is used to handle events of a backgound process.
*/


TBackgroundProcessHandler::TBackgroundProcessHandler(QObject *parent) :
    QObject(parent),
    TDatabaseContext()
{
    moveToThread(Tf::app()->databaseContextMainThread());
}


bool TBackgroundProcessHandler::autoDelete() const
{
    return _autoDelete;
}


void TBackgroundProcessHandler::setAutoDelete(bool autoDelete)
{
    _autoDelete = autoDelete;
}


void TBackgroundProcessHandler::deleteAutoDeleteHandler()
{
    if (_autoDelete) {
        deleteLater();
    }
}
