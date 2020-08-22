---
title: 画像の操作
page_id: "080.070"
---

## 画像の操作

画像処理が可能なライブラリはいろいろ存在します。複雑な画像処理を行う場合は [OpenCV](http://opencv.org/){:target="_blank"} を使用しなければならないかもしれませんが、ここでは Qt ライブラリを使った手軽な方法を解説します。 TreeFrog Framework のベースライブラリである Qt はGUI ツールキットなので、画像処理のための便利な関数がたくさん用意されているのです。

まず、画像の処理を行うためには、QtGUIモジュールを有効にした TreeFrog Framework が必要です。
フレームワークをリコンパイルしましょう。

Linux / Mac OS Xの場合：

```
 $ ./configure --enable-gui-mod
 $ cd src
 $ make
 $ sudo make install
 $ cd ../tools
 $ make
 $ sudo make install
```

Windows の場合：

```
 > configure --enable-debug --enable-gui-mod
 > cd src
 > nmake install
 > cd ..\tools
 > nmake instal
 > cd ..
 > configure --enable-gui-mod
 > cd src
 > nmake install
 > cd ..\tools
 > nmake install
```

次に、Web アプリケーション側にも設定が必要です。プロジェクトファイル(.pro)を編集し、変数QTに "gui" を追加します。

```
  :
 QT += network  sql  gui
  :
```

このように設定すると、アプリは GUI アプリとしてビルドされることになり、特に Linux では実行環境として X Window System が必要になります。<br>
この要件を満たせない場合は、画像処理ライブラリとして OpenCV を使われることをお薦めします。

## 画像のリサイズ

次のコードは、JPEG 画像を、アスペクト比を保ったまま QVGA サイズに変換し保存する例です。

```c++
QImage img = QImage("src.jpg").scaled(320, 240, Qt::KeepAspectRatio);
img.save("qvga.jpg");
```

※ 実際には、ファイルパスとして絶対パスで記述してください。

この QImage クラスを使えば、アスペクト比を無視して変換することも可能ですし、別の画像フォーマットに変換することも容易です。詳細は [Qt ドキュメント](https://doc.qt.io/qt-5/){:target="_blank"}をご覧ください。

## 画像の回転

次のコードは、画像を時計回りに９０度回転させる例です。

```c++
QImage img("src.jpg");
QImage rotatedImg = img.transformed(QMatrix().rotate(90.0));
rotatedImg.save("rotated.jpg");
```

とても簡単です。

## 画像の合成

２つの画像を重ね合わせてみましょう。大きな画像の右上の座標へ小さな画像を合成します。

```c++
QImage background("back.jpg");
QPainter painter(&background);
painter.drawImage(0, 0, QImage("small.jpg"));
background.save("composition.jpg");
```

元の画像へのペインタを用意し、そこへ別の画像を描画しています。