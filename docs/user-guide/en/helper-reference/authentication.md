---
title: Authentication
page_id: "080.020"
---

## Authentication

TreeFrog offers a concise authentication mechanism.
In order to implement the functionality of authentication, you need to create a model class that represents the "user" first. Here, we will try to make a user class with the property of a username and password.
We’ll define a table as follows.

```sql
 > CREATE TABLE user ( username VARCHAR(128) PRIMARY KEY, password VARCHAR(128) );
```

Then navigate to the application root directory and create a model class by generator command.

```
 $ tspawn usermodel user
   created  models/sqlobjects/userobject.h
   created  models/user.h
   created  models/user.cpp
   created  models/models.pro
```
 
By specifying the 'usermodel' option (or you can use 'u' option), the user model class that inherits from TAbstractUser class is created.

The field names of user name and password of the user model class are 'username' and 'password' by default, but you can change them. For example, in the case of defining the schema using the name of 'user_id' and 'pass', make the class after specifying by using the generator command as follows. 

```
 $ tspawn usermodel user user_id pass
```

- You can just simply add it to the end.
 
Unlike a normal class model, authenticate method such as the following have been added to the user model class. This method is used to authenticate the name. The process is to set the user name as a key. Then the method reads the user object, compares the password, and only if it matches, it returns the correct model object.

```c++
User User::authenticate(const QString &username, const QString &password)
{
    if (username.isEmpty() || password.isEmpty())
        return User();
    TSqlORMapper<UserObject> mapper;
    UserObject obj = mapper.findFirst(TCriteria(UserObject::Username, username));
    if (obj.isNull() || obj.password != password) {
        obj.clear();
    }
    return User(obj);
}
```

Please ensure that any modifications are based on the above.
There may be cases where the authentication process is left to an external system. Also there may be cases when, as a password, md5 values need to be saved.

## Login

Let's create a controller that does the login/logout process. As an example, we will create an AccountController class with 3 actions of form, login, and logout.

```
 > tspawn controller account form login logout
   created  controllers/accountcontroller.h
   created  controllers/accountcontroller.cpp
   created  controllers/controllers.pro
```

A skeleton code is generated.
In the form action, we display the login form.

```c++
void AccountController::form()
{
    userLogout();  // forcibly logged out
    render();      // shows form view
}
```

We simply display the form here, but if you have already logged in, it is possible to redirect it to the different screen. The response can be tailored to your requirements.

Now, we'll create a view of the login form using the view file *views/account/form.erb*. Here, login action is the place for the login form to be posted.

```html
<!DOCTYPE HTML>
<html>
<head>
  <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
</head>
<body>
  <h1>Login Form</h1>
  <div style="color: red;"><%==$message %></div>
  <%== formTag(urla("login")); %>
    <div>
      User Name: <input type="text" name="username" value="" />
    </div>
    <div>
      Password: <input type="password" name="password" value="" />
    </div>
    <div>
      <input type="submit" value="Login" />
    </div>
  </form>
</body>
</html>
```
 
In the login action, you can write the authentication process that is executed when user name and password are posted. Once the authentication is successful, call the userLogin() method, and then let the user login to the system.

```c++
void AccountController::login()
{
    QString username = httpRequest().formItemValue("username");
    QString password = httpRequest().formItemValue("password");
 
    User user = User::authenticate(username, password);
    if (!user.isNull()) {
        userLogin(&user);
        redirect(QUrl(...));
    } else {
        QString message = "Login failed";
        texport(message);
        render("form");
    }
}
```
 
- If you do not include the *user.h* file it would cause a compile-time error.

That completes the login process.
Although not included above, it is recommended to call the userLogin() method once logged in, in order to check for any duplicate login. Checking the return value (bool) is therefore advised.
 
After calling userLogin() method, the return value of identityKey() method of user model is stored into the session. By default, a user name is stored.

```c++
 QString identityKey() const { return username(); }
```

You can modify the return value, which should be unique in the system, for example, a primary key or ID, and can get the value by calling identityKeyOfLoginUser() method.
 
## Logout

To log out, all you do is simply to call the userLogout() method in action.

```c++
void AccountController::logout()
{
    userLogout();
    redirect(url("Account", "form"));  // redirect to a login form
}
```

## Checking logging in

If you want to protect access from users who are not logged, you can override the preFilter() of controller, and write the process there.

```c++
bool HogeController::preFilter()
{
    if (!isUserLoggedIn()) {
        redirect( ... );
        return false;
    }
    return true;
}
``` 

When the preFilter() method returns false, the action will not be processed after this.
If you would like to protect the access in many controllers, you can set it to preFilter() of ApplicationController class.ass.
 
## Getting the logged-in user

Let's get an instance of the logged-in user.
You can get the identity information of the logged-in user by the identityKeyOfLoginUser() method. If the return value is empty, it indicates nobody is logging in the session; otherwise the value is a user name string by default.

Define a getter method by the key value in user model class.

```c++
User User::getByIdentityKey(const QString &username)
{
    TSqlORMapper<UserObject> mapper;
    TCriteria cri(UserObject::Username, username);
    return User(mapper.findFirst(cri));
}
```

In a controller, call as follow.

```c++
QString username = identityKeyOfLoginUser();
User loginUser = User::getByIdentityKey(username);
```
 
### Additional Comment

The implementations of login that are described in this chapter all use the session. Therefore, the lifetime of the session will be the lifetime of the login.