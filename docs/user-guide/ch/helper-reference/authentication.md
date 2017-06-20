---
title: 身份验证
page_id: "080.020"
---

## 身份验证

Treefrog提供了一个简单的身份验证机制.<br>
要实现验证功能, 你先需要创建一个模型(model)类表示用户. 这里, 我们将尝试创建一个包含* username*和*password*属性的用户类.
我们按下面这样定义一个表:

```sql
 > CREATE TABLE user ( username VARCHAR(128) PRIMARY KEY, password VARCHAR(128) );
```

然后进入应用程序的根目录, 使用生成器命令创建模型(model)类.

```
 $ tspawn usermodel user
   created  models/sqlobjects/userobject.h
   created  models/user.h
   created  models/user.cpp
   created  models/models.pro
```

通过指定' usermodel'选项(或者使用'u'选项), 继承自TAbstractUser类的用户模型类将被创建.

用户模型(model)类的用户名和密码字段名默认是'username'和'password', 当然, 你可以更改它们. 例如, 如果想使用字段名'user_id'和'pass'定义表结构, 使用生成器命令如下:

```
 $ tspawn usermodel user user_id pass
```

- 你仅需要简单的将这些字段名加在命令后面即可.

不像其他普通类模型, 下面的authentication方法被增加到用户模型类中. 这个方法用来身份验证用户的名字. 出于这个目的, 用户名被设置成关键字来获取用户数据对象. 然后身份验证方法读取用户对象, 比较密码, 只有匹配才返回正确的模型对象.

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

请确保任何修改是基于以上的代码.<br>
可能有身份验证处理用于外部系统. 也有可能是密码需要用md5保存.

## 登录

让我们创建一个控制器(controller)执行login/logout处理. 此例中, 我们将创建一个AccountController类, 有三个操作(action): *form*, *login*, 和*logout*.<br>
下面的代码展示了如何实现:

```
 > tspawn controller account form login logout
   created  controllers/accountcontroller.h
   created  controllers/accountcontroller.cpp
   created  controllers/controllers.pro
```

结果是, 一个代码骨架被生成.<br>
在form操作(action)中, 我们能显示登录的表单.

```c++
void AccountController::form()
{
    userLogout();  // 强制退出
    render();      // 显示表单视图
}
```

在此例中, 我们简单的显示了一个表单, 但如果你已经登录了, 可能/需要重定向到一个不同的界面. 响应可以按照你的需求进行定制.
现在我们将创建一个登录表单的视图, 使用视图文件*views/account/form.erb*. 这里, login操作(action)放置在表单中, 用来post.

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

在login操作(action)中, 你可以写身份验证处理, 这个身份验证处理在用户名和密码提交后执行. 一旦身份验证成功, 调用usrLogin()方法, 然后让用户登录到系统中.

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

- 如果没有包含*user.h*文件将会产生编译错误.

这样就完成了登录处理.<br>
虽然没有包含在上面的代码中, 用户登录后建议调用userLogin()方法一次来检查重复的登录. 检查返回值(bool).

在调用userLogin()方法后, 用户模型的indentitykey()方法的返回值, 保存在会话中.同时, 用户名也被保存.

```c++
 QString identityKey() const { return username(); }
```
你可以修改返回值, 返回值应该在系统中是唯一的. 例如, 你可以返回主键或者ID, 然后通过调用indentityKeyOfLoginUser()方法获得值.

## 退出

要退出, 需要做的就是简单的在操作(action)中调用userLogout()方法.

```c++
void AccountController::logout()
{
    userLogout();
    redirect(url("Account", "form"));  // 重定向到登录表单
}
```

## 检查登录

如果像防止没有登录的用户访问, 你可以重新控制器的preFilter()方法.在那里写处理过程.

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

当preFilter()方法返回*false*, 操作(action)不会在它后面执行.<br>
如果你想在许多个控制器(controller)上限制访问, 可以将代码写在ApplicationController类的preFilter().

##  获取已登录用户

首先, 我们需要获取一个已登录用户的实例. 可以使用identityKeyOfLoginUser()方法获得已登录用户的识别信息. 如果返回值是空的, 表示没有会话中用户没有登录, 否则这个值是默认字符类型的用户名.

接下来, 在返回关键字的用户模型类定义一个getter方法.

```c++
User User::getByIdentityKey(const QString &username)
{
    TSqlORMapper<UserObject> mapper;
    TCriteria cri(UserObject::Username, username);
    return User(mapper.findFirst(cri));
}
```

在控制器中, 使用下面的代码:

```c++
QString username = identityKeyOfLoginUser();
User loginUser = User::getByIdentityKey(username);
```

### 额外注释

在这一章中, 写的登录实现都是使用会话. 因此, 会话的生命周期和登录的生命周期是相同的.