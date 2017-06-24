---
title: Cookie
page_id: "050.020"
---

## Cookie

Cookies are information stored in the browser (PC) containing key-value pairs which are supposed to be shared between the browser and the online Web application. There are basically some restrictions, but the functions of cookies are supported by almost all browsers. Cookies are essential for establishing sessions as described in the previous section.

The Cookie system was originally proposed by Netscape Communications Corporation and published as an RFC before becoming standardized.

## Save a String in a Cookie

Cookies are created by the Web application which the user has access to. In accordance with the specifications cookies, they are saved as key-value pairs for the HTTP response.

When the relevant action occurs, the following functions are called and the key-value pairs will be saved inside a cookie.

```c++
addCookie("key1", "Hello world.");
```

An expiration date can be set on a cookie as well. The cookie rides on the HTTP request, and it is sent between server and browser. Once it reaches the expiration date the cookie will be automatically erased from the browser and eventually disappears. The expiration date is specified in the third argument of the addCookie() function. For more information, refer to the [API reference](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}.

## Reading a String from a Cookie

By using the keys, values can be retrieved from the cookies present in the HTTP request in the following way.

```c++
QByteArray text = httpRequest().cookie("key1");
// text = "Hello world."
```