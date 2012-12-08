#include <QtTest/QtTest>
#include "tlogsender.h"
#include "tlogfilewriter.h"

#define LOG_SERVER_NAME_PREFIX  QLatin1String(".tflogsvr_")


class BenchMark : public QObject
{
    Q_OBJECT
private slots:
    void sendLog();
    void writeFileLog();
};


void BenchMark::sendLog()
{
    TLogSender sender(LOG_SERVER_NAME_PREFIX + "treefrog");
    //sender.waitForConnected();

    QBENCHMARK {
        sender.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        sender.writeLog("o0sa8dufassljf6823648236826");
        sender.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        sender.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        sender.writeLog("o0sa8dufassljf6823648236826");
        sender.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        sender.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        sender.writeLog("o0sa8dufassljf6823648236826");
        sender.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
    }
}


void BenchMark::writeFileLog()
{
    QString file = "temp.log";
    TLogFileWriter writer(file);
    QBENCHMARK {
        writer.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        writer.writeLog("o0sa8dufassljf6823648236826");
        writer.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        writer.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        writer.writeLog("o0sa8dufassljf6823648236826");
        writer.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        writer.writeLog("aildjfliasjdl;fijaswelirjas;lidfja;lsiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
        writer.writeLog("o0sa8dufassljf6823648236826");
        writer.writeLog("aildjfliasjdl;fijaswusdifhsidfhskdjfhksuhfkhwkefrkwjnfksjdnfkunknkiwejf;alsdf;lakjer;liajds;flkasd;lfaj;esrldfij");
    }

    QFile::remove(file);
}

QTEST_MAIN(BenchMark)
#include "benchmarking.moc"
