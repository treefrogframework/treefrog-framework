---
title: Access Routing
page_id: "050.040"
---

## Access Control

Access control in a website can be considered in two ways. It can be any control by the user authentication, or control by connecting host (IP address). For control by the host, please set for the Web server (Apache/nginx) as required.

First, I will discuss control by the user.

## User Access Control

On some Web sites, there is a fixed page that anyone can access, and also pages that can only be accessed by users. For example an Admin page would only be accessible to persons with the right authority. In such cases, you can deny access in the following ways:

First, refer to the section on [authentication]({{ site.baseurl }}/user-guide/en/helper-reference/authentication.html){:target="_blank"} and create a user model class.
For the page to which you want to restrict access, login authentication should be required. By doing so, an instance of the user model will be obtained by doing so.

Then override setAccessRules of controller() method. Set the access rules of access allow/deny the action by group or user ID. User ID and group point to the user model classes of identityKey() method and groupKey() method respectively for the return value.

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

Use allowUser() and allowGroup() to allow access, and define denyUser() and denyGroup() to deny access. The first argument specifies the group or user ID; the second argument specifies the list or action name (QStringList).

For permissions/denials from the user that do not define the access rules, the default settings will be used. Use setAllowDefault() method or setDenyDefault() method for this.

```c++
 setDenyDefault(true);
```
 
Here is the logic to validate users with access. This overrides preFilter() method of the controller, and returns false when the user denies the access. The controller preFilter() method overrides to deny access the user if false is returned.

```c++
bool FooController::preFilter()
{
   ApplicationController::preFilter();
    :
    :   // Get an instance of the user model
    :
   if (!validateAccess(&loginUser)) {  // to verify the access rules you have defined
       renderErrorResponse(403);
       return false;
   }
   return true;
}
```

If the preFilter() method returns false, the action will not be executed. You will therefore want to deny access. You can then use renderErrorResponse() method to display a static error page (*public/403.html* in this example).