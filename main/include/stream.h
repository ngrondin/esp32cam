#include <stdlib.h>

#ifndef STREAM_H
#define STREAM_H

void init_stream();
int stream_start(char* ip, uint8_t id);
void stream_stop();

#endif