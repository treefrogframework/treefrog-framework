---
title: faq
page_id: "faq.00"
---

## FAQ

Frage: Gibt es ein Docker-Image für das TreeFrog-Framework?

Antwort: Ja, die Images dafür wurden auf Docker Hub veröffentlicht: [https://hub.docker.com/r/treefrogframework/](https://hub.docker.com/r/treefrogframework/){:target="_blank"}


Frage: Ich erhalte folgende Fehlermeldung: `Incorrect string value: '\xE3\x81\x82' for column 'xxxx' at row 1 QMYSQL: Unable to execute query` bei MySQL. Weswegen erhalte ich diese Fehlermeldung?

Antwort: Es handelt sich dabei um ein Zeichensatzproblem, bitte überprüfen Sie den entsprechenden Zeichensatz Ihrer Tabelle. Zur Vermeidung solcher Fehler sollten Sie den Default-Zeichensatz beim Erstellen der Tabelle mittels `DEFAULT CHARSET=UTF8`` gleich mitangeben.