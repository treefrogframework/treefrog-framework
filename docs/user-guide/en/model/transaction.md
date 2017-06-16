---
title: Transaction
page_id: "060.030"
---

## Transaction

If the DBMS supports transaction, the transaction will work by default; then, when you launch the app, it is already working.

The framework starts the transaction just before the action is called, and after being called, it commits the transaction.

If something abnormal occurs and you want to roll back the transaction, you can throw an exception or call rollbackTransaction() method in the controller. A Rollback should be implemented after completing the action.

```c++
// in an action
  :
if (...) {
    rollbackTransaction();
      :
}
```

If you don't want to activate the transaction itself deliberately, you can override the transactionEnabled() method of the controller and then return *false*.

```c++
bool FooController::transactionEnabled() const
{
    return false;
}
```

You can set each controller, as in this example, or, if you don't want to use it all, you can override in ApplicationController.

```c++
bool ApplicationController::transactionEnabled() const
{
    return false;
}
```