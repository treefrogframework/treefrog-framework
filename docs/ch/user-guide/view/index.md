---
title: 视图(view)
page_id: "070.0"
---

## 视图(view)

视图(view)在网页应用中角色是在反馈中生成一个HTML文档. 开发者为每个HTML文件创建一个模版, 这些模版将嵌入从控制器(controller)模型(model)传递过来的变量.

目前Treefrog框架采用了两个模版系统. 它们是Otama和ERB. 两个系统都可以使用模版动态输出值, 并且执行循环和分支.

ERB系统像在Ruby中一样, 用来在编程中嵌入模版的代码. 这种机制它有比较容易理解的优点, 但是检查和修改网页设计比较困难.

Otama是一种完全分离界面逻辑和模版的一种模版系统. 好处是容易检验或更改网页设计, 编程者和界面设计者可以更好的协作. 缺点是需要一些比较复杂的机制, 这些机制会有少量的学习成本.

两者的性能是相同的.要代码被构建时, 在模版转换成C++代码后, 模版被编译. 共享库(动态连接库)会被创建, 所以它们都运行得比较快. 然后, 它取决与用户的代码的内容.

* [链接到ERB模版系统>>]({{ site.baseurl }}/ch/user-guide/view/erb.html){:target="_blank"}
* [链接到Otama模版系统>>]({{ site.baseurl }}/ch/user-guide/view/otama-template-system.html){:target="_blank"}