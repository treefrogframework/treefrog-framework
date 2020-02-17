---
title: 数据验证
page_id: "080.030"
---

## 数据验证

有时, 请求发送的数据可能不是开发者定义的格式. 例如, 一些用户可能在需要数字的地方输入了字母. 即使客户端用脚本实现了数据验证, 篡改请求的内容并不复杂, 因此服务端进行内容验证是必须的.

如[控制器(controller)]({{ site.baseurl }}/ch/user-guide/controller/index.html){:target="_blank"}中提到的, 接收的请求的数据是Hash格式.通常在发送请求数据给模型(model)前, 每个值都应该验证值的格式.

首先, 我们将为验证*blog*的请求数据(hash)生成一个validation类骨架. 进入应用程序根目录, 然后执行下面的命令.

```
$ tspawn validator blog
  created   helpers/blogvalidator.h
  created   helpers/blogvalidator.cpp
  updated   helpers/helpers.pro
```

验证规则设置在生成的BlogValidator类的构造中. 下面的例子展示了title变量至少4个字符最多20个字符的规则的实现方式.

```c++
BlogValidator::BlogValidator() : TFormValidator()
{
   setRule("title", Tf::MinLength, 4);
   setRule("title", Tf::MaxLength, 20); 
}
```

枚举值是第二个参数. 你可以定义强制输入, 最大/最小字符长度, 最大/最小整形值, 日期格式, e-mail地址格式, 用户定义的规则(正则表达式)等(这些规则定义在tfnamespach.h).

setRule()也有第四个参数.它用来设置验证器错误信息.如果你没有定义一个信息(像我们上面的例子), 将使用*config/validation.ini*中定义的消息.

规则是隐式设置为"强制输入的". 如果不想输入为"强制", 可将规则写成这样:

```c++
setRule("title", Tf::Required, false);
``` 

<div class="center aligned" markdown="1">

**规则**

</div>

<div class="table-div" markdown="1">

| 枚举值       | 含义              |
|--------------|-------------------|
| Required     | 需要输入值        |
| MaxLength    | 字符串最大长度    |
| MinLength    | 字符串最小长度    |
| IntMax       | 整形最大值        |
| IntMin       | 整形最小值        |
| DoubleMax    | 双整形最大值      |
| DoubleMin    | 双整形最小值      |
| EmailAddress | Email 地址格式    |
| Url          | URL 格式          |
| Date         | 日期格式          |
| Time         | 时间格式          |
| DateTime     | 日期时间格式      |
| Pattern      | 正则表达式        |

</div><br>

一旦你设置了规则, 就可以在控制器(controller)中使用它们. 包含关于这个的头文件.<br>
下面的代码例子验证了从表单获取的请求数据. 如果验证错误, 将得到一个错误信息.

```c++
QVariantMap blog = httpRequest().formItems("blog");
BlogValidator validator;
if (!validator.validate(blog)) {
   //获取验证器的错误信息
   QStringList errs = validator.errorMessages();
    :
}
```

通常情况下, 因为有多个规则, 也就会有多个错误信息. 一个一个地处理有点太麻烦了. 然而, 如果使用下面的方法, 你可以一次导出所有的验证错误信息(传递给视图(view)).

```c++
exportValidationErrors(valid, "err_");
``` 

第二个参数, 为导出对象定义了一个变量名的前缀.

##### 概要: 给表单数据设置规则, 然后使用validate()验证.

## 客户化验证

上面的解释是关于静态验证.它不能用与动态的情况, 如根据值的不同允许不同的值范围. 这种情况下, 你可以重新validate方法, 然后你可以写任何代码来做验证.

下面的代码是如何客户化验证的一个例子:

```c++
bool FooValidator::validate(const QVariantMap &hash)
{
    bool ret = THashValidator::validate(hash);  // 验证静态内容
    if (ret) {
        QDate startDate = hash.value("startDate").toDate();
        QDate endDate = hash.value("endDate").toDate();
        
        if (endDate < startDate) {
            setValidationError("error");
            return false;
        }
          :
          :
    }
    return ret;
}
```

它比较*endData*和*startDate*的值. 如果endDate小于startDate, 将会抛出错误信息.