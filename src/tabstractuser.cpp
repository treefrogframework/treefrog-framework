#include "tabstractuser.h"


/*!
  \class TAbstractUser
  \brief The TAbstractUser class is the abstract base class of users,
  providing functionality common to users.
*/


/*!
    \fn virtual TAbstractUser::~TAbstractUser() 
    Destroys the user object.
*/

    
/*!
   \fn virtual QString TAbstractUser::identityKey() const = 0
   Returns the identity key, such as username.
  
   This is a pure virtual function.
*/
