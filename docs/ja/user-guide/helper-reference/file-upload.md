---
title: ファイルのアップロード
page_id: "080.010"
---

## ファイルのアップロード

ファイルのアップロードのためのフォームをつくりましょう。下はERBで書く例です。formTag() メソッドの第３引数に true を指定することで、 multipart/form-data のフォームが生成されます。

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

この例では、ファイルのアップロード先は同じコントローラの upload アクションになります。

アップロードファイルを受け取るアクションでは、次のメソッドを使うことでそのファイルをリネームすることができます。アップロードファイルは一時ファイルの扱いなので、リネームしないとアクション終了後にファイルは自動的に削除されます。

```c++
TMultipartFormData &formdata = httpRequest().multipartFormData();
formdata.renameUploadedFile("picture", "dest_path");
```

元のファイル名は次のメソッドで取得することができます。

```c++
QString origname = formdata.originalFileName("picture");
```

あまり使われないかもしれませんが、次のメソッドでアップロード直後の一時ファイル名を取得することができます。重複しないようにランダムなファイル名が付けられています。

```
QString upfile = formdata.uploadedFilePath("picture");
```

## 可変個のファイルのアップロード

アップロードするファイルの数があらかじめ決まっていれば、上記の方法で対応することができますが、決められないケースもありえます。JavaScript ライブラリを使えば、可変個のファイルをアップロードすることが容易にできます。<br>
TreeFrog Framework はこのようなケースにも対応しています。以下の例では、分り易くするために２個のファイルの場合で説明します。

次のようにフォームを作成します。

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

JavaScript で動的にinputタグを作成する場合、name="picture[]" というように名前の末尾に "[]" をつけるのがポイントです。

アップロードファイルを受け取る uploadアクションでは、次のように TMimeEntity オブジェクトを通して２つのファイルにアクセスできます。イテレータを使用するのがポイントです。

```c++
QList<TMimeEntity> lst = httpRequest().multipartFormData().entityList( "picture[]" );
for (QListIterator<TMimeEntity> it(lst); it.hasNext(); ) {
    TMimeEntity e = it.next();
    QString oname = e.originalFileName();   // 元のファイル名
    e.renameUploadedFile("dst_path..");     // リネームファイル
      :
}
```