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
   Returns the identity key, such as a user name.
  
   This is a pure virtual function.
*/


/*!
   \fn virtual QString TAbstractUser::groupKey() const 
   Returns the group key, such as a group name.
  
   This is a virtual function.
*/
