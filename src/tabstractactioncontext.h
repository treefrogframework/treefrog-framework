#pragma once
class TActionController;
class THttpRequest;


class T_CORE_EXPORT TAbstractActionContext {
public:
    virtual ~TAbstractActionContext() {}
    virtual const TActionController *currentController() const = 0;
    virtual THttpRequest &httpRequest() = 0;
    virtual const THttpRequest &httpRequest() const = 0;
};
