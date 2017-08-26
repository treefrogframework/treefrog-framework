---
title: トランザクション
page_id: "060.030"
---

## トランザクション

トランザクションをサポートした DBMS であれば、デフォルトでトランザクションが機能します。アプリを起動すれば、もうすでに機能しているのです。

フレームワークは、アクションが呼び出される直前にトランザクションを開始し、呼び出された後にトランザクションをコミットします。

もし何らかの異常が発生しトランザクションをロールバックしたい場合は、例外をスローするか、コントローラの中で rollbackTransaction メソッドを呼んでください。アクションから抜けたあとで、ロールバックが実行されます。

```c++
// in an action
  :
if (...) {
    rollbackTransaction();
      :
}
```

敢えてトランザクションそのものを作動させたくなければ、コントローラの transactionEnabled() メソッドをオーバライドして false を返します。

```c++
bool FooController::transactionEnabled() const
{
    return false;
}
```

この例のようにコントローラ毎にも設定できますし、全く使用しなければ ApplicationController でオーバライドすることができます。

```c++
bool ApplicationController::transactionEnabled() const
{
    return false;
}
```