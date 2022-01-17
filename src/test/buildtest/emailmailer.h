#pragma once

#include <TActionMailer>

class EmailMailer : public TActionMailer
{
public:
    EmailMailer() { }

    void sendEmail(const QString &user);
};
