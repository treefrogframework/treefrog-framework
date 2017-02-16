---
title: Cookie
page_id: "050.020"
---

## Cookie

Cookies are information stored in the browser (PC) as a pair (keys and value) to be shared between the browser and the Web application side. There are some restrictions, but cookies are a function supported by almost all browsers. They are essential for operating sessions as described in the previous section.

The Cookie system was originally proposed by Netscape Communications Corporation and published as an RFC before becoming standardized.
 

## Save a String in a Cookie

Cookies are produced by the Web application. In accordance with the specifications of the cookie, they are saved as key-value pairs for the HTTP response.

When the relevant action occurs, the following functions are called, and the key-value pairs will be saved to cookies.

```c++
 addCookie("key1", "Hello world.");
```

An expiration date can be set on the cookie. The cookie rides on the HTTP request, and is sent between server and browser. Once it reaches the expiration date the cookie is automatically erased from the browser, and disappears. This is specified in the third argument of addCookie() function expiration date. For more information, refer to the [API reference](http://treefrogframework.org/tf_doxygen/classes.html){:target="_blank"}.
 
## Reading a String from a Cookie

Using the key, values can be retrieved from the cookies present in the HTTP request, as follows.

```c++
 QByteArray text = httpRequest().cookie("key1");
  // text = "Hello world."
```