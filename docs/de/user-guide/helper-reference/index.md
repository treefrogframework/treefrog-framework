---
title: Helper Reference
page_id: "080.0"
---

## Helper Reference

An helper is a helping function or class to complement the treatment. In TreeFrog Framwork there are many helpers, so that by using them, code can be simple and easy to understand. The following helpers are examples of their kind:

* User authentication
* Form validation
* Mailer (Sending mail)
* Access to the upload file
* Logging
* Access control of users

If you make a helper as a class, it can be basically a class without a state (as per object-oriented). In a class with a state, the general case is defined as a model since it persists in the DB.

In addition, for a similar piece of logic which appears several times in the controller and the view, you should consider whether it is worth cutting it out as a helper. Please see [this chapter](/en/user-guide/helper-reference/making-original-helper.html){:target="_target"} if you want to create a helper on your own.