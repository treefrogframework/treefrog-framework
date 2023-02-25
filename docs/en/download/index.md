---
title: download
page_id: "download.00"
---

## Download

### Installer for Windows

TreeFrog Installer for Qt 6 has been released. If installing it, the development environment of TreeFrog Framework will be constructed immediatly. In advance, install Qt 6 for Windows.

<div class="table-div" markdown="1">

| Version                           | File                                   |
|-------------------------------------|--------------------------------------|
| 2.7.0 for Visual Studio 2019 (Qt 6.4 or 6.3)| [<i class="fa fa-download" aria-hidden="true"></i> treefrog-2.7.0-msvc_64-setup.exe](https://github.com/treefrogframework/treefrog-framework/releases/download/v2.7.0/treefrog-2.7.0-msvc_64-setup.exe) |

</div>

## Source Code

The source code packages of TreeFrog Framework are available.

<div class="table-div" markdown="1">

| Source         | File                             |
|----------------|----------------------------------|
| version 2.7.0 | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-framework-2.7.0.tar.gz](https://github.com/treefrogframework/treefrog-framework/archive/v2.7.0.tar.gz) |

 </div>

[All previous versions <i class="fa fa-angle-double-right" aria-hidden="true"></i>](https://github.com/treefrogframework/treefrog-framework/releases)

Latest source code is in [GitHub](https://github.com/treefrogframework/).

## Homebrew

See Homebrew [site](https://formulae.brew.sh/formula/treefrog) of TreeFrog.
Can install by Homebrew on macOS.

```
 $ brew install treefrog
```

If the SQL driver for Qt was installed correctly, the followings is displayed.

```
 $ tspawn --show-drivers
 Available database drivers for Qt:
   QSQLITE
   QMARIADB
   QMYSQL
   QPSQL
```

If `QMARIADB` or `QPSQL` is not displayed, the driver is not stored in the correct directory. Run the following commands to check the path of the driver directory and copy the drivers there manually.

```
Example:
 $ cd $(tspawn --show-driver-path)
 $ pwd
 (your_brew_path)/Cellar/qt/6.2.3_1/share/qt/plugins/sqldrivers
```

After copying the driver, the following results are obtained when checked with the `ls` command.

```
$ ls $(tspawn --show-driver-path)
libqsqlite.dylib  libqsqlmysql.dylib  libqsqlpsql.dylib
```
