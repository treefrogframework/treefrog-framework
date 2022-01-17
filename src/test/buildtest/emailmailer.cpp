#include "emailmailer.h"

static EmailMailer mailer;


void EmailMailer::sendEmail(const QString &user)
{
    texport(user);
    deliver("authEmailChange");
}
