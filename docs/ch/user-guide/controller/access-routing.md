---
title: 访问控制
page_id: "050.040"
---

## 访问控制

访问控制在网站中可以通过两种方式实现:

* 通过用户授权(User Authorization)和
* 通过连接主机(IP address)

通过主机控制, 请按需设置网页服务器(Apache/nginx).

首先, 我们将要谈谈如何通过控制用户访问网站.

## 用户访问控制

在一些网站上, 有一些网页是任何人都可以访问的, 而一些网页是只能特定的用户访问. 例如, 管理页面只能有特定授权的人才能访问. 这样的例子, 你可以按如下方法禁止访问:

首先, 参考[授权(authentication)]({{ site.baseurl }}/ch/user-guide/helper-reference/authentication.html){:target="_blank"} 并建立一个用户模型(user model)类.<br>
你希望限制访问的页面,  登录授权(login authentication)是需要的.通过这样, 获得一个用户模型(user model)的实例.

然后重写controller()的setAccessRules方法. 通过组(Group)或用户(User ID)设置任意的操作(action)为*allow*或者*deny*.*User ID*和Group*都指向用户模型(user model)类.当用户执行一个操作(action)时,模型类中的identityKey()和groupKey()方法返回当前允许或禁止.

```c++
void FooController::setAccessRules()
{
   setDenyDefault(true);
   QStringList allowed;
   allowed << "index" << "show" << "entry" << "create";
   setAllowUser("user1", allowed);
    :
}
```

使用allowUser()和allowGroup()来允许访问, 使用denyUser()和denyGroup()来禁止访问. 第一个参数指定组(Group)或者用户(User Id), 第二个参数指定操作的列表(QStringList).
对于没有定义的允许/禁止, 将使用默认的设置. 使用setAllowDefault()和setDenyDefault()方法进行默认设置.

```c++
setDenyDefault(true);
```

这里是使用户可以访问的逻辑. 这下面的方法重写了控制器(controller)的preFilter()方法, 然后在用户被禁止访问的情况下返回*false*. 控制器preFilter()方法在用户没有授权的情况下返回*false*.

```c++
bool FooController::preFilter()
{
   ApplicationController::preFilter();
    :
    :   // 获得用户模型(user model)实例
    :
   if (!validateAccess(&loginUser)) {  // 检查访问规则, 是否被禁止访问
       renderErrorResponse(403);
       return false;
   }
   return true;
}
```

如果preFilter()方法返回false, 操作(action)将不会被执行. 你会希望能看到访问被禁止的信息. 为此, 你可以使用renderErrorRepose()方法显示一个静态的错误页面(如*public/403.html*).