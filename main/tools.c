#include "include/tools.h"
#include <sys/time.h>
#include "include/general.h"

cron_handler_t cron_handler;

void timer_start(tool_timer_t* timer) {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t start = (uint64_t)tv_now.tv_sec * 1000000L + (uint64_t)tv_now.tv_usec;
    timer->start = start;
}

void timer_end(tool_timer_t* timer) {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    timer->end = (uint64_t)tv_now.tv_sec * 1000000L + (uint64_t)tv_now.tv_usec;
    timer->dur = (timer->end - timer->start) / 1000L;
}

void cron() {
    while(true) {
        if(cron_handler) {
            cron_handler();
        }
        vTaskDelay(delay120000ms);
    }
}

void init_cron(cron_handler_t handler) {
    cron_handler = handler;
    TaskHandle_t xHandle = NULL;
    static uint8_t ucParameterToPass;
    xTaskCreate( cron, "cam_loop", 2000, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
}