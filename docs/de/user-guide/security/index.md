---
title: Security
page_id: "100.0"
---

## Security

Once you publish the website, you have to understand the threats it faces and set security measures. The application developer has to program carefully in order to avoid vulnerability on the web. You can find detailed articles on the internet, that's why I'll not go to deept into detail here.

Since TreeFrog has an inbuilt security system, you can establish a safe website when using it properly.

## SQL Injection Prevention

As you will know, SQL injection means that due to the problems of SQL sentence construction, the database can be attacked or misused. TreeFrog advises observance of the following SQL injection measures:

* Use ORM object => you can make safe SQL query sentences inside.
* Use TSqlQuery place holder when constructing SQL sentences => the values are processed by escape automatically.
* When constructing SQL sentences by string concatenation, use TSqlQuery::escapeIdentifier() for field name, and TSqlQuery::formatValue() for value. => escape processing is done for each.

## Cross-site Scripting Prevention (CSRF)

If there is a deficiency in the process of generating websites, it can allow the inclusion of malicious scripts, and if that happens, a cross-site scripting attack can be established. In order to guard against this, you should implement a policy of using escape for all values and attribute outputs which output dynamically into the view area. In the TreeFrog, the following is done:

* The eh() method or '<%= .. %>' notation should be used for outputting values.

You have to be very careful if you output the area WITHOUT using the escape processing, that is the echo() method or '<%= .. %>' notation.

Basically, for the content of any \<script\> .. \</script\> element, I recommend you to not dynamically output any information dependent on outside input.

## CSRF Protection

Any site that accepts user requests without any verification, has a possible vulnerability to CSRF (Cross-Site Request Forgery) exists. If an HTTP request has been trumped by a third party, you have to discard it.

To prevent this, perform the followings both.

 * As for the information on the form, ONLY accept requests in the POST method.
 * Put a row of letters that is difficult to predict as a hidden parameter on one of the information items within the form, and validate it when receiving it. => by generating a form tag with the formTag() method, the hidden parameter is automatically granted.

If this hidden parameter is easily predictable, this CSRF protection is insufficient. This parameter is a string that has been converted by the hash function, using the Session.Secret parameter, in the *application.ini* configuration file. It is therefore sufficiently difficult for the string to be guessed by anyone not knowing the *Session.Secret* value.

In order to turn on the CSRF protection, you need to set the following in the *application.ini* file:

```
 EnableCsrfProtectionModule=true
```

I would recommend that you turn this feature off during the development of the application.

##### In brief: Generate a form tag using the formTag() method.

## Session Hijacking Prevention

When a session ID is easily guessed or stolen, spoof-like acts are possible. This kind of spoof-like conduct is called session hijacking.

In the TreeFrog Framework, as a defense against session hijacking, a new session ID of sufficient length to avoid being guessed is generated using random number and a hash function each time the site is accessed. In that way the session ID is extremely difficult to guess and, for a general site, provides what is thought to be adequate protection.

Although guessing a session ID is difficult, a more serious threat is possible eavesdropping on the network. To counter this, it is possible to encrypt (SSL) the communication channel by reverse proxy (such as nginx or Apache).

##### In brief: Encrypt by SSL all sites where important information is being dealt with.
