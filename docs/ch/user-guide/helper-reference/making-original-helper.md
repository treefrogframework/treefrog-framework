---
title: 创建基类工具助手
page_id: "080.090"
---

## 创建基类工具助手

创建一个工具助手后, 它可以在模型(model)/控制器(controller)/视图(view)中访问. 当相似的代码出现几次后, 请考虑创建一个工具助手.

## 如何创建工具助手

你可以在*helpers*文件夹中按你想的去实现这个类, 记住使用T_HELPER_EXPORT宏. 在那以后你可以做任何事.

```c++
#include <TGlobal>
class T_HELPER_EXPORT SampleHelper
{
    //写你想要的代码
};
```

增加头文件和源文件到项目文件*helpers.pro*, 然后执行make.

## 如何使用工具助手

包含头文件后, 可以像普通类一样使用它. 它和普通类相比, 没有什么特别的不同.