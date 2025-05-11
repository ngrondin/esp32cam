#include <esp_err.h>
#include "esp_camera.h"


#ifndef CAM_H
#define CAM_H

typedef enum {
    CAM_MODE_MONITOR,
    CAM_MODE_HTTPSTREAM,
    CAM_MODE_UDPSTREAM,
    CAM_MODE_PICTURE         
} cam_mode_t;

esp_err_t init_camera();
void camera_set_mode(cam_mode_t m);
esp_err_t camera_set_size(int size);
esp_err_t camera_set_flip(int f);
esp_err_t camera_set_auto(int a);
esp_err_t camera_set_gain(int g);
esp_err_t camera_set_ae(int l);
esp_err_t camera_set_aec(int l);
esp_err_t camera_set_brightness(int b);

cam_mode_t camera_get_mode();

#endif