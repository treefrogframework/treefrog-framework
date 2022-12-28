#include <TCommandLineInterface>


static int command()
{
    Tf::setupAppLoggers(new TStdOutLogger);
    Tf::setAppLogLayout("%d %5P %m%n");
    Tf::setAppLogDateTimeFormat("yyyy/MM/dd hh:mm:ss");

    tInfo("Start");
    tError("Test error message");
    tWarn("Test warning message");
    Tf::msleep(100);
    tInfo("End");
    return 0;
}

TF_CLI_MAIN(command)
