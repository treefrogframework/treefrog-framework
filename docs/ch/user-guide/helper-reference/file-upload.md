---
title: 文件上传
page_id: "080.010"
---

## 文件上传

在这一章, 我们将创建一个简单的表单来上传文件. 下面的代码使用ERB. 同时指定formTag()方法的第三个参数为*true*, 表单将会按照multipart/form-data生成.

```
<%== formTag(urla("upload"), Tf::Post, true) %>
  <p>
    File: <input name="picture" type="file">
  </p>
  <p>
    <input type="submit" value="Upload">
  </p>
</form>
```

此例中, 文件上传的地址是在同一个控制器(controller)下的upload操作(action).

在操作(action)中接收到的上传文件也可以通过下面的方法重命名:

```c++
TMultipartFormData &formdata = httpRequest().multipartFormData();
formdata.renameUploadedFile("picture", "dest_path");
```

上传的文件将视为临时文件. 如果你重命名上传的文件, 这个文件会在操作(action)结束后自动删除.
原始的文件名可以通过下面的方式获得:

```c++
QString origname = formdata.originalFileName("picture");
```

它可能用得不多, 不过你可以在使用uploadedFilePath()方法后获得一个临时的文件名. 随机的文件名可以确保文件不会重叠覆盖.

```c++
QString upfile = formdata.uploadedFilePath("picture");
```

## 上传多个文件

Treefrog框架同样支持上传多个文件. 你可以使用JavaScript库来上传多个文件. 这里, 我将介绍一种比较简单的方法来上传2个文件(或者多个)

首先,我们创建下面的表单:

```
<%== formTag(urla("upload"), Tf::Post, true) %>
  <p>
    File1: <input name="picture[]" type="file">
    File2: <input name="picture[]" type="file">
  </p>
  <p>
    <input type="submit" value="Upload">
  </p>
</form>
```

当使用JavaScript动态创建一个Input标签, 增加"[]"到'name'后是非常重要的, 例如name="picture[]".

要在upload操作(action)中接收上传的文件, 你可以通过TMimeEntity对象访问这两个文件,如下:

```c++
QList<TMimeEntity> lst = httpRequest().multipartFormData().entityList( "picture[]" );
for (QListIterator<TMimeEntity> it(lst); it.hasNext(); ) {
    TMimeEntity e = it.next();
    QString oname = e.originalFileName();   // 原始文件名
    e.renameUploadedFile("dst_path..");     // 重命名
      :
}
```

别忘了在这里使用迭代器.

