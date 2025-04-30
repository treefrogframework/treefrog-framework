---
title: View
page_id: "070.0"
---

## View

The role of a view in a web application is to generate an HTML document and return it as a response. Developers create HTML templates in which variables passed from the controller are embedded in predefined locations. These templates can also include conditional branches and loops.

TreeFrog allows the use of ERB as its template system. ERB is a system (or library) that lets you write program code directly within templates, similar to what is available in Ruby. While ERB is simple and easy to understand, it has the drawback of making it harder to preview or modify web designs.

When code written with the template system is built, it is converted into C++ code, compiled, and turned into a single shared library (dynamic link library). Since it is C++, it runs with high performance.

By delegating the frontend to a JavaScript framework, the backend implementation can be kept minimal. TreeFrog can generate a scaffold for Vite + Vue.

Although a template system called Otama, which separates templates from presentation logic, was implemented, it is now deprecated.

* [To the chapter of ERB template system >>]({{ site.baseurl }}/en/user-guide/view/erb.html){:target="_blank"}
* [To the chapter of Vite + Vue >>](/en/user-guide/view/vite+vue.html){:target="_blank"}
