---
title: File Upload
page_id: "080.010"
---

## File Upload

Let's make a form for file upload. Below is an example of writing in ERB. By specifying true for the third argument of the formTag() method, a form as multipart/form-data will be generated.

```html
<%== formTag(urla("upload"), Tf::Post, true) %>
  <p>
    File: <input name="picture" type="file">
  </p>
  <p> 
    <input type="submit" value="Upload">
  </p>
</form>
```
 
In this example, the destination of the file upload is the upload action in the same controller.

In the action that receives the upload file, you can rename the file by using the following method. The uploaded file is treated as a temporary file; if you do rename it now, the file is automatically deleted after the action ends.

```c++
TMultipartFormData &formdata = httpRequest().multipartFormData();
formdata.renameUploadedFile("picture", "dest_path");
```

The original file name can be obtained using the following method.

```c++
QString origname = formdata.originalFileName("picture");
```
 
It may not be much used, but you can get a temporary file name just after uploading by using the uploadFile() method. Random file names have been attached to make sure that they do not overlap.

```c++
QString upfile = formdata.uploadedFile("picture");
``` 

## Upload a variable number of files

TreeFrog Framework also supports uploading a variable number of files. You can upload files of variable number when you use the Javascript library. Here, I’ll explain an easier way of uploading two files (or more).

First we create a form as follows.

```html
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

To receive the uploaded file in the upload action, you can access the two files through the TMimeEntity object as follows. But don’t forget here using the Iterator for that.

```c++
QList<TMimeEntity> lst = httpRequest().multipartFormData().entityList( "picture[]" );
for (QListIterator<TMimeEntity> it(lst); it.hasNext(); ) {
    TMimeEntity e = it.next();
    QString oname = e.originalFileName();   // original name
    e.renameUploadedFile("dst_path..");     // rename file
      :
}
```