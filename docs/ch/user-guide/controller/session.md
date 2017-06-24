---
title: 会话(Session)
page_id: "050.010"
---

## 会话(Session)

在一个网站上, 各种信息将会从一个页面带到另外一个页面. 一个熟习的例子就是购物车中已经选择的产品在各个页面中传递.

HTTP协议不会保留任何信息(它被称为无状态的). 因此, 为了保留这些信息需要某种机制. 这个机制称之为*会话(Session)*, 它由框架或者语言提供.

- 此外, 从另外一种意义上说, 还有这样的例子, 网站的用户进行在离开网站前进行"一系列的通信".

## 给会话(Session)增加数据

要给会话(Session)增加数据, 可这样做:

```
 session().insert("name", "foo");
 或
 session().insert("index", 123);
```

在操作(action)中访问会话(Session), 我们也可以这样写:
```
 session()["name"] = "foo";
 或
 session()["index"] = 123;
```

## 从会话(Session)中读取数据

可以这样从会话(Session)中读取数据:

```
 QString name = session().value("name").toString();
 或
 int index = session().value("index").toInt();
```

我们也可以这样写:
 ```
 QString name = session()["name"].toString();
 或
 int index = session()["index"].toInt();
```

## 设置会话(Session)的保存位置

到目前为止, 会话可以看成是一个键值对的"联合数组(Hash)", 这些数据表现为字符串形式. 会话本身是一个对象, 为了在页面间携带这些信息, 我们需要将信息保存在某个地方.

在Treefrog框架内, 你可以选择一个文件, 数据库(RDB SQlObject), cookies作为会话内容的保存位置. Session类已经实现这些功能. 存储类型在*apliction.ini*中设置.

## 设置用Cookies保存会话(Session)

如果喜欢用Cookies保存, 可以简单的写成:

```
 Session.StoreType=cookie
```
 
通过保存cookies, 会话的内容将会保存在客户端(浏览器), 在你允许的情况下, 用户还可以获得这些内容. 信息不应该显示给用户的, 应该保存在服务器(例如, 数据库RDB). 作为规则, 你应该仅将最小的必要信息保存在会话(Session)中.
## 设置用文件保存会话(Session)
如果你希望设置一个cookie保存文件, 可以简单的写成这样(会话文件将持续的写如应用程序根目录下的*tmp*文件夹内):

```
 Session.StoreType=file
```

如果应用服务器(AP server)并行运行在多个机器上, 这种方法用得不多. 因为从用户来的请求不会总是指向同一个应用服务器(AP server)上, 使用会话文件保存需要一个共享文件服务器. 再者, 为了正确安全的操作, 文件锁机制也是需要的.

## 设置用数据库RDB保存会话(Session)

前提条件是, *database.ini*文件已经设置好了数据库信息.<br>
为了保存会话(Session)到数据库RDB, 需要一张表. 因此, 按下面的方法用SQL语句在数据库中创建'session'表:
MySQL范例:

```sql
 > CREATE TABLE session (id VARCHAR(50) PRIMARY KEY, data BLOB, updated_at TIMESTAMP);
```

*applicaiton.ini*文件也应该修改

```
 Session.StoreType=sqlobject
```

以上就是我们需要做的. 如果需要, 系统可以保存会话(Session)的其他信息到数据库.

## 会话(Session)的生命周期

会话的有效期(用秒计算)设置在配置文件的*Session.Lifetime*. 如果超过失效日期, 会话信息将被擦除或者销毁. 此外, 可以指定为0, 表示只要浏览器一直运行, 会话信息就有效. 这种情况下, 当浏览器关闭后会话信息被销毁.

### 列

每个浏览器分配一个会话. 每个不同的电脑有不同的会话, 并且同一台电脑上不同的浏览器有不同的会话.

框架持续监控会话并给每个会话分配一个数字.

一个唯一的ID(很难猜测)将会分配给每个会话(Session). 这个ID以cookie的形式在浏览器内存储, 然后附加在HTTP请求上发送, 直到超过失效日期. 框架然后从存储位置根据ID找到会话(Session)信息.