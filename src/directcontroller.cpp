/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "directcontroller.h"


void DirectController::show(const QString &view)
{
    setLayoutEnabled(false);
    render(view);
}

T_REGISTER_CONTROLLER(directcontroller)


/*!
  \class DirectController
  \brief The DirectController class is for internal use only.
*/
