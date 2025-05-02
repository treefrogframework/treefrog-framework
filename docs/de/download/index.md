---
title: download
page_id: "download.00"
---

## Download

### Installationsassistent für Windows

Wir stellen einen Installationsassistenten für Qt6 zur Verfügung. Nach der Einrichtung verfügen Sie im Handumdrehen über eine TreeFrog-Framework-Entwicklungsumgebung. Es ist nicht erforderlich, den Quellcode zu bauen und zu installieren, daher ist dieser Installationsassistent für diejenigen geeignet, die schnell eine Umgebung erstellen möchten. Vor der Ausführung des Installationsassistenten muss Qt6 für Windows installiert werden.

<div class="table-div" markdown="1">

| Version                           | Datei                                  |
|-------------------------------------|--------------------------------------|
| 2.7.1 für Visual Studio 2019 (Qt 6.4 or 6.3)| [<i class="fa fa-download" aria-hidden="true"></i> treefrog-2.7.1-msvc_64-setup.exe](https://github.com/treefrogframework/treefrog-framework/releases/download/v2.7.1/treefrog-2.7.1-msvc_64-setup.exe) |

</div>

## Quellcode

Das Quellcode-Paket des TreeFrog-Frameworks ist ebenfalls verfügbar.

<div class="table-div" markdown="1">

| Source         | Datei                            |
|----------------|----------------------------------|
| Version 2.7.1 | [<i class="fa fa-download" aria-hidden="true"></i> treefrog-framework-2.7.1.tar.gz](https://github.com/treefrogframework/treefrog-framework/archive/v2.7.1.tar.gz) |

 </div>

[Alle vorhergehenden Versionen <i class="fa fa-angle-double-right" aria-hidden="true"></i>](https://github.com/treefrogframework/treefrog-framework/releases)

Der Letztstand des Quellcodes befindet sich auf [GitHub](https://github.com/treefrogframework/).

## Homebrew

Siehe die Homebrew-[Seite](https://formulae.brew.sh/formula/treefrog) von TreeFrog. Dieser kann mittels Homebrew auf macOS-basierten Geräten wie folgt installiert werden:

```
 $ brew install treefrog
```

Wenn der SQL-Treiber für Qt korrekt installiert wurde, dann werden die folgenden Ausgaben angezeigt:

```
 $ tspawn --show-drivers
 Available database drivers for Qt:
   QSQLITE
   QMARIADB
   QMYSQL
   QPSQL
```

Wenn `QMARIADB` oder `QPSQL` nicht angezeigt werden sollten, dann sind die jeweiligen Treiber nicht am dafür vorgesehenen Speicherort abgelegt. Führen Sie folgende Befehle aus um den entsprechenden Treiber-Pfad zu überprüfen und kopieren Sie den Treiber manuell in den dafür vorgesehenen Ordner:

```
Example:
 $ cd $(tspawn --show-driver-path)
 $ pwd
 (your_brew_path)/Cellar/qt/6.2.3_1/share/qt/plugins/sqldrivers
```

Nach dem Kopieren des Treibers sollten die folgenden Ausgaben mittels des `ls`-Befehls angezeigt werden:

```
$ ls $(tspawn --show-driver-path)
libqsqlite.dylib  libqsqlmysql.dylib  libqsqlpsql.dylib
```
