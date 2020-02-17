---
title: Access Routing
page_id: "050.040"
---

## Access Control

Access control in a website can be done in two ways:

* by user authenticiation and
* by the connecting host (IP address).

For access control by the host, please set for the Web server (Apache/nginx) as required.

First of all, we will talk about how to control user access for Web sites.

## User Access Control

On some Web sites, there is a fixed amount pages that anyone can access, and also pages that can only be accessed by users. For example an Admin page would only be accessible to persons with the right authority. In such cases, you can deny access in the following ways:

First, refer to the section on [authentication]({{ site.baseurl }}/en/user-guide/helper-reference/authentication.html){:target="_blank"} and create a user model class.<br>
For the page to which you want to restrict access, login authentication should be required. By doing so, an instance of the user model will be obtained.

Then override setAccessRules of the controller() method. Set the access rules for any the action by group or user ID to *allow* or *deny*. Both *User ID* and *Group* point to the user model class. The identityKey() method and the groupKey() method in the model class each return the value that represents granted or denied access when the user is performing an action.

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

Here is the logic to validate users with access. The following method here overrides the preFilter() method of the controller and returns *false* when the user access has been denied. The controller preFilter() method returns *false* if user access is not granted.

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

If the preFilter() method returns false, the action will not be executed. You will therefore want to visualize that the access hs been denied. For this purpose, you can then use the renderErrorResponse() method to display a static error page (*public/403.html* in this example).