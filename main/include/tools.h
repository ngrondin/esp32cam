#include <stdlib.h>

#ifndef TOOLS_H
#define TOOLS_H

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t dur;
} tool_timer_t;

typedef void (*cron_handler_t)(); 

void timer_start(tool_timer_t* timer);
void timer_end(tool_timer_t* timer);
void init_cron(cron_handler_t handler);

#endif