/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include "logserver.h"


static void usage()
{
    fprintf(stderr, "usage: tflogsvr app-name log-file\n");
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Sets codec
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    QStringList args = QCoreApplication::arguments();
    LogServer server(args.value(1), args.value(2), &app);
    if (!server.start()) {
        usage();
        return 1;
    }
    return app.exec();
}
