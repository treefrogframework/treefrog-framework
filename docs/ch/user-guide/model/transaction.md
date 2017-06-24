---
title: 事务(Transaction)
page_id: "060.030"
---

## 事务(Transaction)

如果数据库支持事务(Transaction), 事务(Transaction)默认会工作, 当你加载应用的时候, 它已经工作了.

框架在操作(action)被调用前开始事务,被调用完成后提交事务.

如果发生了不正常的状况, 你希望回滚事务, 你可以在控制器内抛出一个异常或者调用rollbackTransaction()方法. 回滚应该在完成操作(action)后实现.

```c++
//在操作(action)内
   :
 if (...) {
      rollbackTransaction();
       :
}
```

如果不想让事务(transaction)自己激活, 你可以重写控制器的transactionEnabled()方法,然后返回*false*.

```c++
bool FooController::transactionEnabled() const
{
   return false;
}
```

你可以设置每一个控制器, 在这个例子中, 如果你根本不想使用事务, 你可以在applicationController中重写.

```c++
bool ApplicationController::transactionEnabled() const
{
   return false;
}
```