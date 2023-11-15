---
title: Making Original Helper
page_id: "080.090"
---

## Making Original Helper

When you create a helper, it can be accessible from the model/controller/view. Please try to consider making a helper whenever similar code appears several times.

## How to Make the helper

You can implement the class as you like in the *helpers* directory, remember to put the charm of T_HELPER_EXPORT. Other than that you can do anything.

```c++
#include <TGlobal>
class T_HELPER_EXPORT SampleHelper
{
    // Write as you like.
};
```

Add the header and the source file to project files, *helpers.pro*, then run make.

## How to Use the Helper

Use it the same way as a normal class by including a header file. There is no particular difference compared to the way the normal class is used.