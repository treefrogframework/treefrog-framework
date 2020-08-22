---
title: 图像生成
page_id: "080.070"
---

## 图像生成

有许多能够进行图像处理的库. 如果需要进行复杂的图像处理, 你可能需要使用[OpenCV](http://opencv.org/){:target="_blank"}, 不过在这章我将介绍使用Qt库来生成图像. 它是非常容易使用的. 因为Treefrog框架是基于Qt的, Qt已经提供了一个包含很多有用的函数的图形处理的GUI工具包.

首先, 需要激活Treefrog框架的QTGUI模块.<br>
出于这个目的, 让我们重新编译框架.

在Linux / Mac OS X中 :

```
$ ./configure --enable-gui-mod
$ cd src
$ make
$ sudo make install
$ cd ../tools
$ make
$ sudo make install
```

在Windows中:

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

接下来, 网页应用端的这个设置也是需要的. 编辑项目文件(.pro), 增加"gui"到变量*QT*中.

```
  :
 QT += network  sql  gui
  :
```

这样的设置, 应用将按照GUI应用进行构建. 对于Linux, 特别是X Window Systems是需要实现这个环境的. 如果不满足需求, 建议使用OpenCV作为图像处理库.

## 改变图像大小

下面的代码是一个如何通过转换到QVGA尺寸同时保持长宽比例保存JPEG图像的例子:

```c++
 QImage img = QImage("src.jpg").scaled(320, 240, Qt::KeepAspectRatio);
 img.save("qvga.jpg");
```
- 在实际中, 使用绝对路径作为文件的路径.

使用QImage类, 你可以忽略长宽比进行转换. 你也可以转换成不同的图像格式. 详细信息请看[Qt 文档](https://doc.qt.io/qt-5/){:target="_blank"}.

## 选择图像

下面的代码是将图像顺时针旋转90度的例子.

```c++
QImage img("src.jpg");
QImage rotatedImg = img.transformed(QMatrix().rotate(90.0));
rotatedImg.save("rotated.jpg");
```

## 图像合成

让我们重叠两个图像. 你可以将小图片对齐到大图片坐标的右侧.

```c++
QImage background("back.jpg");
QPainter painter(&background);
painter.drawImage(0, 0, QImage("small.jpg"));
background.save("composition.jpg");
```

你可以准备一个painter给原始的图像, 然后在上面绘制一个不同的图片.