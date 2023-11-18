---
title: Startseite
page_id: "home.00"
---

## <i class="fa fa-bolt" aria-hidden="true"></i> Klein, aber leistungsstark und effizient

Das TreeFrog Framework ist ein Hochperformanz- und Full-Stack-C++-Framework für die Entwicklung von Webanwendungen, welches die HTTP- und WebSocket-Protokolle unterstützt.

Webanwendungen können schneller ausgeführt werden als Skriptsprachen, da das serverseitige Framework in C++/Qt geschrieben wurde. Bei der Anwendungsentwicklung stellt es ein objektrelationales Mapping-System und ein Templating-System auf einer MVC-Architektur bereit und zielt darauf ab höchste Produktivität, durch Konvention über Konfiguration, zu erreichen.


## <i class="fa fa-flag" aria-hidden="true"></i> Kennzeichnungsmerkmale

  1. Hochperformanz - hoch-optimierte Anwendungsserver-Engine auf C++-Basis.
  2. Objektrelationales Mapping - kapselt komplexe und umständliche Datenbankzugriffe
  3. Templating-System - Adapation einer ERB-ähnlichen Template-Engine
  4. Unterstützte Datenbanken - MySQL, MariaDB, PostgreSQL, ODBC, MongoDB, Redis, Memcached, etc.
  5. WebSocket-Unterstützung - stellt vollduplex Kommunikationskanäle bereit
  6. Generator - generiert Codegerüst, Makefile und vue.js - Vorlagen
  7. Unterstützung für unterschiedliches Rückgabetypen - JSON, XML und CBOR
  8. Multiplatform-Fähigkeit - derselbe Quellcode kann somit auf Windows, macOS und Linux funktionieren
  9. OSS - quelloffene Software, nun mit BSD-Lizenz


## <i class="fa fa-comment" aria-hidden="true"></i> Weswegen das TreeFrog-Framework?

Es wird gesagt, dass es bei der Entwicklung von Webanwendungen einen Kompromiss zwischen Entwicklungseffizienz und Betriebsgeschwindigkeit gibt. Stimmt das wirklich?

So etwas gibt es nicht. Durch die Bereitstellung praktischer Entwicklungswerkzeuge und hervorragender Bibliotheken aus dem Framework, sowie durch die Angabe von Spezifikationen zur Minimierung der Konfigurationsdatei, ist eine effiziente Entwicklung möglich.

In den letzten Jahren ist das Cloud-Computing aufgekommen, die Bedeutung von Webanwendungen nimmt von Jahr zu Jahr zu. Obwohl bekannt ist, dass die Ausführungsgeschwindigkeit von Skriptsprachen mit zunehmender Codemenge abnimmt, kann die Sprache C++ mit der höchsten Geschwindigkeit und einem geringen Speicherbedarf arbeiten ohne dabei die Ausführungsgeschwindigkeit zu vermindern, selbst wenn die Codemenge zunimmt.

Mehrere Anwendungsserver, die Skriptsprache ausführen, können ohne Leistungseinbußen zu einem zusammengefasst werden. Probiere das TreeFrog-Framework aus, das hohe Produktivität und Hochgeschwindigkeitsbetrieb kombiniert!


## <i class="fa fa-bell" aria-hidden="true"></i> Neuigkeiten

Mar. 26, 2023
### TreeFrog Framework version 2.7.1 (stable) release <span style="color: red;">New!</span>

  - Fix a bug of opening shared memory KVS.
  - Modified to reply NotFound when it can not invoke the action.

  [<i class="fas fa-download"></i> Download this version](/en/download/)

Feb. 25, 2023
### TreeFrog Framework version 2.7.0 (stable) release

  - Fix possibility of thread conflicting when receiving packets.
  - Changed hash algorithm to HMAC of SHA3.
  - Added Memcached as session store.
  - Updated malloc algorithm of TSharedMemoryAllocator.
  - Updated system logger.
  - Performance improvement for pooling database connections.

Jan. 21, 2023
### TreeFrog Framework version 2.6.1 (stable) release

 - Fix a bug of outputting access log.
 - Added a link option for LZ4 shared library on Linux or macOS.

Jan. 2, 2023
### TreeFrog Framework version 2.6.0 (stable) release

 - Implemented in-memory KVS for cache system.
 - Added a link option for Glog shared library.
 - Fix bugs of macros for command line interface.
 - Updated LZ4 to v1.9.4.

Nov. 1, 2022
### TreeFrog Framework version 2.5.0 (stable) release

 - Implemented flushResponse() function to continue the process after sending a response.
 - Updated glog to v0.6.0
 - Performance improvement for redis client.
 - Implemented memcached client. [Experimental]
 - Implemented a cache-store for memcached, TCacheMemcachedStore class.

Aug. 13, 2022
### TreeFrog Framework version 2.4.0 (stable) release

 - Implemented memory store for cache.
 - Updated Mongo C driver to v1.21.2.

May 28, 2022
### TreeFrog Framework version 2.3.1 (stable) release

 - Fix compilation errors on Qt 6.3.

 [<i class="fa fa-list" aria-hidden="true"></i> All changelogs](https://github.com/treefrogframework/treefrog-framework/blob/master/CHANGELOG.md)


## <i class="fas fa-hand-holding-usd"></i>Unterstütze die Entwicklung mit deiner Spende

Das TreeFrog-Framework ist ein New-BSD-lizenziertes Open-Source-Projekt und kann völlig kostenlos verwendet werden. Allerdings ist der Aufwand, der für die Wartung und Entwicklung neuer Funktionen für das Projekt erforderlich ist, ohne angemessene finanzielle Unterstützung nicht tragbar. Wir nehmen Spenden von Sponsoren und Einzelspendern über die folgenden Methoden entgegen:

 - Spende mittels [PayPal <i class="fas fa-external-link-alt"></i>](https://www.paypal.me/aoyamakazuharu)
 - werde ein [Sponsor in GitHub](https://github.com/sponsors/treefrogframework)
 - Bitcoin-Spendenadresse: [12C69oSYQLJmA4yB5QynEAoppJpUSDdcZZ]({{ site.baseurl }}/assets/images/btc_address.png "Bitcoin-Spendenadresse")

Ich würde mich freuen, wenn du dir überlegen würdest mit deiner Spende das TreeFrog-Framework-Projekt zu unterstützen. Danke schön!


## <i class="fa fa-user" aria-hidden="true"></i>Wir suchen ...

 - Entwickler, Tester, Übersetzer.

Da diese Website mit [GitHub Pages](https://pages.github.com/) erstellt wurde, können Übersetzungen auch per Pull-Request übermittelt werden.
Besuchen Sie [GitHub](https://github.com/treefrogframework/treefrog-framework){:target="_blank"}. Du bist herzlich willkommen!


## <i class="fa fa-info-circle" aria-hidden="true"></i> Weiterführende Informationen

[TreeFrog forum <i class="fas fa-external-link-alt"></i>](https://groups.google.com/forum/#!forum/treefrogframework){:target="_blank"}

Twitter [@TreeFrog_ja <i class="fas fa-external-link-alt"></i>](https://twitter.com/TreeFrog_ja){:target="_blank"}

[Docker Images <i class="fas fa-external-link-alt"></i>](https://hub.docker.com/r/treefrogframework/treefrog/){:target="_blank"}

[Benchmarks <i class="fas fa-external-link-alt"></i>](https://www.techempower.com/benchmarks/){:target="_blank"}
