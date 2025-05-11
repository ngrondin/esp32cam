#include <stdlib.h>


#ifndef MONITOR_H
#define MONITOR_H

typedef void (*monitor_handler_t)(uint8_t diff); 

void init_monitor(monitor_handler_t handler);
uint8_t* monitor_get_cur_bmp();
uint8_t* monitor_get_ref_bmp();
uint8_t* monitor_get_dif_bmp();
uint8_t monitor_get_luminosity();
void monitor_reset_settling();

#endif