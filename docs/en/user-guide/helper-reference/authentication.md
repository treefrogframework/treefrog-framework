---
title: Authentication
page_id: "080.020"
---

## Authentication

TreeFrog offers a concise authentication mechanism.<br>
In order to implement the functionality of authentication, you need to create a model class that represents the "user" first. Here, we will try to make a user class containing the properties *username* and *password*.
We'll define a table as follows:

```sql
 > CREATE TABLE user ( username VARCHAR(128) PRIMARY KEY, password VARCHAR(128) );
```

Then navigate to the application root directory and create a model class by using the generator command:

```
 $ tspawn usermodel user
   created  models/sqlobjects/userobject.h
   created  models/user.h
   created  models/user.cpp
   created  models/models.pro
```

By specifying the 'usermodel' option (or you can use 'u' option), the user model class, that inherits from TAbstractUser class, will be created.

The field names of user name and password of the user model class are 'username' and 'password' and they are created by default, but, of course, you can change them. For example, in case you want to define a schema using the field names 'user_id' and 'pass' instead, use the generator command as follows:

```
 $ tspawn usermodel user user_id pass
```

- You just simply need to add those field names at the end of the command.

Unlike a normal class model, an authentication method, such as the following, has been added to the user model class. This method is used to authenticate the user's name. For this purpose, the user name is set the key for retriving the user data object. Then the authentication method reads the user object, compares the password, and only if it matches, it returns the correct model object.

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

Please ensure that any modifications are based on the code above.<br>
There may be cases where the authentication process is left to an external system. Also there may be cases when a password values need to be saved in md5.

## Login

Let's create a controller that does the login/logout process. In this example, we will create an AccountController class with three actions: *form*, *login*, and *logout*.<br>
The following code shows how to achieve this:

```
 > tspawn controller account form login logout
   created  controllers/accountcontroller.h
   created  controllers/accountcontroller.cpp
   created  controllers/controllers.pro
```

As a result, a skeleton code is generated.<br>
In the form action, we can display the login form in the view.

```c++
void AccountController::form()
{
    userLogout();  // forcibly logged out
    render();      // shows form view
}
```

In this example, we simply display the form, but if you have already logged in, it is possible/necessary to redirect to a different screen. The response can be tailored to your requirements.

Now, we'll create a view of the login form using the view file *views/account/form.erb*. Here, login action is the place for the login form to be posted.

```
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

In the login action, you can write the authentication process that is executed when a user name and a password are posted. Once the authentication was successful, call the userLogin() method and then let the user login to the system.

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

That completes the login process.<br>
Although not included in the code above, it is recommended to call the userLogin() method once the user is logged in order to check for any duplicate login(s). Checking the return value (bool) is therefore advised.

After calling the userLogin() method, the return value of identityKey() method of user model is stored into the session. By default, a user name is stored.

```c++
 QString identityKey() const { return username(); }
```

You can modify the return value, which should be unique in your system. For example, you can let return a primary key or ID and then can get the value by calling the identityKeyOfLoginUser() method.

## Logout

To log out, all you need to do is simply to call the userLogout() method in the action.

```c++
void AccountController::logout()
{
    userLogout();
    redirect(url("Account", "form"));  // redirect to a login form
}
```

## Checking Logging In

If you want to prevent access from users who are not logged, you can override the preFilter() of controller. Write that process there.

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

When the preFilter() method returns *false*, the action will not be processed after this.<br>
If you would like to restrict access in many controllers, you can set it to preFilter() of the ApplicationController class.

## Getting the Logged-in User

First of all, we need to get an instance of the logged-in user. You can get the identity information of the logged-in user by the identityKeyOfLoginUser() method. If the return value is empty, it indicates that nobody is logging in the session; otherwise the value is a user name from type string by default.

Next, define a getter method in user model class that returns the key.

```c++
User User::getByIdentityKey(const QString &username)
{
    TSqlORMapper<UserObject> mapper;
    TCriteria cri(UserObject::Username, username);
    return User(mapper.findFirst(cri));
}
```

In the controller, use the following code:

```c++
QString username = identityKeyOfLoginUser();
User loginUser = User::getByIdentityKey(username);
```

### Additional Comment

The implementations of login that are described here in this chapter all using the session. Therefore, the lifetime of the session will be simultaneously the lifetime of the login.