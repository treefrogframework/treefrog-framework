---
title: File Upload
page_id: "080.010"
---

## File Upload

In this chapter we will create a simple form for a file upload. The code example below is using ERB. By specifying *true* for the third argument of the formTag() method, a form will be generated as multipart/form-data.

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

In this example, the destination of the file upload is the upload action in the same controller.

The upload file that is being received in action can be renamed as well by using the following method:

```c++
TMultipartFormData &formdata = httpRequest().multipartFormData();
formdata.renameUploadedFile("picture", "dest_path");
```

The uploaded file is treated as a temporary file. If you do rename the upload file, the file is automatically deleted after the action ends.<br>
The original file name can be obtained using the following method:

```c++
QString origname = formdata.originalFileName("picture");
```

It may not be much used, but you can get a temporary file name just after uploading by using the uploadedFilePath() method. Random file names have been attached to make sure that they do not overlap.

```c++
QString upfile = formdata.uploadedFilePath("picture");
```

## Upload a variable number of files

TreeFrog Framework also supports uploading a variable number of files. You can upload a variable number files provided that you use the Javascript library. Here, I'll explain an easier way of uploading two files (or more).

First we create a form as follows:

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

When creating an input tag with JavaScript dynamically, it is important to add "[]" at the end of the 'name' like name = "picture []".

To receive the uploaded files in the upload action, you can access the two files through the TMimeEntity object as follows:

```c++
QList<TMimeEntity> lst = httpRequest().multipartFormData().entityList( "picture[]" );
for (QListIterator<TMimeEntity> it(lst); it.hasNext(); ) {
    TMimeEntity e = it.next();
    QString oname = e.originalFileName();   // original name
    e.renameUploadedFile("dst_path..");     // rename file
      :
}
```

Don't forget to use the Iterator here for that.
