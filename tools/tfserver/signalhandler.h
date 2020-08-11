#pragma once
namespace TreeFrog {

void setupSignalHandler();
void setupFailureWriter(void (*writer)(const void *data, int size));

}  // namespace TreeFrog
