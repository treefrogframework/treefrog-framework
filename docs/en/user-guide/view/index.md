---
title: View
page_id: "070.0"
---

## View

The role of view in a Web application is to generate an HTML document from a response. The developer creates each template for an HTML document to be embedded in a predetermined portion of the variable passed from the controller (model).

There are two template systems so far adopted into the TreeFrog Framework. These are Otama and ERB. In both systems you can output the value dynamically using the template, and perform loops and conditional branching.

The ERB system, such as in Ruby, is used to embed the template code in the program (library). While there are benefits in that it is easy to understand as a mechanism, it is difficult to check or change the Web design.

Otama is a template system that completely separates the presentation logic and the templates. The advantage is that checking or changing the Web Design is easy, and programmers and designers are better able to collaboration. The disadvantage is that a little more complex mechanism is required, and that there will be a slight learning cost.

Performance is the same in either. When the codes are built, the templates are compiled after it is converted to C++ code. One shared library (dynamic link library) is created, so both run faster. After that, it depends on user's description of the code.

* [To the chapter of ERB template system >>]({{ site.baseurl }}/en/user-guide/view/erb.html){:target="_blank"}
* [To the chapter of Otama template system >>]({{ site.baseurl }}/en/user-guide/view/otama-template-system.html){:target="_blank"}