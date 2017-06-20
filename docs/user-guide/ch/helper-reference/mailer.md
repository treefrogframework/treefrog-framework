---
title: 邮件程序
page_id: "080.040"
---

## 邮件程序

Treefrog框架实现了邮件程序, 它能够使用SMTP发送邮件.目前为止(v1.0), 只能发送SMTP信息. 要发送邮件信息, 需要用到ERB模版.

首先, 使用下面的命令创建邮件骨架:

```
 $ tspawn mailer information send
   created  controllers/informationmailer.h
   created  controllers/informationmailer.cpp
   created  controllers/controllers.pro
   created  views/mailer/mail.erb
```

InformationMailer类在controller文件夹中被创建, 还有一个名为*mail.erb*的模版创建在views文件夹中.

接着, 打开先前创建的*mail.erb*, 然后将内容改成这样:

```
 Subject: Test Mail
 To: <%==$ to %>
 From: foo@example.com
 
 Hi,
 This is a test mail.
```

空行上面是邮件头, 下面是邮件主体. 在邮件头中指定主题和收件人. 它能够增加邮件头的任何字段. 然而, Content-Type和Date字段是自动添加的, 所以不需要在这里写上它们.

如果使用多字节的字符, 例如中文, 使用配置文件中的InternalEncoding参数的设置的编码保存文件(默认是UTF-8).

在InformationMailer类的send()方法的结尾调用deliver()方法.

```c++
void InformationMailer::send()
{
    QString to = "sample@example.com";
    texport(to);
    deliver("mail");   // <-mail.erb 邮件被模版发送了
}
``` 

现在你可以在类的外面调用. 在操作(action)中写下面的代码, 邮件发送的过程将会被执行:

```c++
 InformationMailer().send();
```

- 实际发送邮件时, 请看下面的"SMTP设置"章节.

当你不使用模版直接发送邮件时, 可以使用TSmtpMailer::send()方法.

## SMTP设置

这里是SMTP的配置信息. 必须在*application.ini*文件中设置好SMTP信息.

```ini
# 指定连接的主机名或者IP地址
ActionMailer.smtp.HostName=smtp.example.com
# 指定连接的端口
ActionMailer.smtp.Port=25
# 如果true, 打开SMTP授权, 如果false, 关闭SMTP授权.
ActionMailer.smtp.Authentication=false
# 指定SMTP授权的用户名
ActionMailer.smtp.UserName=
# 指定SMTP授权的密码
ActionMailer.smtp.Password=
# 如果true, 打开邮件延时发送功能. deliver()方法仅将邮件添加到队列中,因此这个方法不会阻塞.
ActionMailer.smtp.DelayedDelivery=false
```

如果使用SMTP授权, 你需要设置这个:

```ini
ActionMailer.smtp.Authentication=true
```

因为授权的方法, CRAM-MD5, LOGIN,和PLAIN(使用这个级别)是一起安装的, 所以授权处理是自动进行的.

在这个框架中, SMTPS邮件发送是不支持的.

## 延时发送邮件

因为用SMTP的方式发送邮件, 需要通过外部的服务器传递数据, 这个过程需要时间. 你可以在邮件发送处理执行前返回一个HTTP响应.

编辑*applicaition.ini*文件如下:

```ini
ActionMailer.smtp.DelayedDelivery=true
```

这样, deliver()方法在合并队列数据时不会发生阻塞. 邮件发送的过程将在返回HTTP响应后执行.

**额外说明:**
如果没有设置延时发送(*false*情况下), deliver()方法会保持阻塞, 直到SMTP处理完成, 或者发生错误.