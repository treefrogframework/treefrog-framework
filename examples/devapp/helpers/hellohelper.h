#ifndef HELLOHELPER_H
#define HELLOHELPER_H

#include <TGlobal>


class T_HELPER_EXPORT HelloHelper
{
public:
    HelloHelper() { }
    HelloHelper(const HelloHelper &other) : name_(other.name_), address_(other.address_) { }
    ~HelloHelper() { }
    
    void setName(const QString &name) { name_ = name; }
    QString name() const { return name_; }
    void setAddress(const QString &address) { address_ = address; }
    QString address() const { return address_; }

private:
    QString name_;
    QString address_;
};


Q_DECLARE_METATYPE(HelloHelper)

#endif // HELLOHELPER_H
