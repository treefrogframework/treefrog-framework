---
title: 工具助手参考
page_id: "080.0"
---

## 工具助手(Helper)参考

一个工具助手是实现处理的一个工具函数或者类. 在Treefrog框架中,有很多的工具助手, 所以通过使用它们, 代码可以变得简单和容易理解. 这些工具助手有下面的这些类型:

* 用户授权(User authentication)
* 表单验证(Form validation)
* 邮件程序(Sending mail)
* 访问上传的文件(Access to the  upload file)
* 记录(Logging)
* 用户访问控制(Access control of users)

如果你想创建一个工具助手类, 它可以是没有状态的一个类(按照面向对象).至于有状态的类, 通常的例子是作为一个模型(model)定义, 因为它保存在数据库中.

此外, 对于出现在控制器(controller)和视图(view)中出现了多次的类似的逻辑代码段, 你应该考虑是否值得将它们剪切下来通过工具助手实现. 如果你想创建自己的工具助手,请看[本章](/ch/user-guide/helper-reference/making-original-helper.html){:target="_target"}