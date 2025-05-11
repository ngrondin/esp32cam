#include <esp_err.h>

#ifndef CONFIG_H
#define CONFIG_H

extern int config_ready;
extern char* wifi_ssid;
extern char* wifi_pass;
extern char* mqtt_uri;
extern char* mqtt_queue_base;
extern char* cam_name;
extern char* ip_str;
extern char* mqtt_cmd_queue;
extern char* mqtt_stat_queue;
extern char* mqtt_move_queue;
extern uint16_t threshold;
extern uint16_t hpthresh;
extern uint8_t* threshmap;

esp_err_t read_config();

esp_err_t write_config();

esp_err_t input_config();

esp_err_t init_config();

#endif