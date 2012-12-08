#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

namespace TreeFrog {

void setupSignalHandler();
void setupFailureWriter(void (*writer)(const void *data, int size));

} // namespace TreeFrog
#endif // SIGNALHANDLER_H
