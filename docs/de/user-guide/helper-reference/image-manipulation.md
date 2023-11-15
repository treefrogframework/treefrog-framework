---
title: Image Manipulation
page_id: "080.070"
---

## Image Manipulation

There are various libraries that are capable of image processing. If you need to do complicated image processing, you may need to use [OpenCV](http://opencv.org/){:target="_blank"}, but in this chapter I would like to explain about image manipulation using the Qt library. It's easy to use. Since Qt is the base library of the TreeFrog Framework, it is ready to provide a GUI tool kit including many useful functions for image processing.

At first, it is necessary to activate the QtGUI module of the TreeFrog framework.<br>
For this purpose, let's recompile the framework.

In the case of Linux / Mac OS X :

```
 $ ./configure --enable-gui-mod
 $ cd src
 $ make
 $ sudo make install
 $ cd ../tools
 $ make
 $ sudo make install
```

In the case of Windows :

```
 > configure --enable-debug --enable-gui-mod
 > cd src
 > nmake install
 > cd ..\tools
 > nmake install
 > cd ..
 > configure --enable-gui-mod
 > cd src
 > nmake install
 > cd ..\tools
 > nmake install
```

Next, this setting is also required for the Web application side. Edit the project file (.pro), add "gui" in the variable with the name *QT*.

```
  :
 QT += network  sql  gui
  :
```

In this setting, the app is built as a GUI application. For Linux, particularly, X Window System is required to implement the environment.If you cannot meet this requirement, it is recommended that you use OpenCV as your image processing library.

## Resize the Image

The following code is an example of how to save a JPEG image by converting to the QVGA size while keeping the aspect ratio:

```c++
QImage img = QImage("src.jpg").scaled(320, 240, Qt::KeepAspectRatio);
img.save("qvga.jpg");
```

- In reality, use the absolute path as a the file path.

Using this QImage class, you can convert while ignoring aspect ratio. Also you can convert to a different image format. Please see [Qt Document](https://doc.qt.io/qt-5/){:target="_blank"} for detail.

## Rotation of the Image

The following code is an example of rotating an image clockwise through 90 degrees.

```c++
QImage img("src.jpg");
QImage rotatedImg = img.transformed(QMatrix().rotate(90.0));
rotatedImg.save("rotated.jpg");
```

## Image Synthesis

Let's superimpose two images. You can align the small image to the coordinates of the upper right corner in the big picture.

```c++
QImage background("back.jpg");
QPainter painter(&background);
painter.drawImage(0, 0, QImage("small.jpg"));
background.save("composition.jpg");
```

You can then prepare a painter to the original image, and draw a different picture there.