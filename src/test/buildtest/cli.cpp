#include <TCommandLineInterface>


static int command()
{
    Tf::setupAppLoggers(new TStdOutLogger);
    Tf::setAppLogLayout("{} %5P %m%n");
    Tf::setAppLogDateTimeFormat("yyyy/MM/dd hh:mm:ss");

    Tf::info("Start");
    Tf::error("Test error message");
    Tf::warn("Test warning message");
    Tf::msleep(100);
    Tf::info("End");
    return 0;
}

TF_CLI_MAIN(command)
