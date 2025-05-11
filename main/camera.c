#include "esp_camera.h"
#include "include/camera.h"
#include "include/general.h"

#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_log.h" //for showing logs
#include "esp_event.h" //for wifi event
#include "driver/gpio.h"
#include <soc/gpio_periph.h>

#define CAM_PIN_PWDN 32  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define MONITOR_FRAMESIZE FRAMESIZE_128X128

sensor_t * sensor;
cam_mode_t cam_mode;
int stream_frame_size;


static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000, ///20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_SVGA, //Start with largest size as otherwise the driver will crash when changing to a larger size

    .jpeg_quality = 12, //0-63, 
    .fb_count = 2,      
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


esp_err_t init_camera()
{
    cam_mode = CAM_MODE_MONITOR;
    stream_frame_size = FRAMESIZE_VGA;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        printf("Camera Init Failed\n");
        return err;
    }
    sensor = esp_camera_sensor_get();
    vTaskDelay(delay500ms);
    camera_set_mode(CAM_MODE_MONITOR);
    return ESP_OK;
}

void camera_set_mode(cam_mode_t mode) {
    cam_mode = mode;
    if(cam_mode == CAM_MODE_MONITOR) {
        printf("Camera in monitor mode\n");
    } else if(cam_mode == CAM_MODE_PICTURE) {
        printf("Camera in picture mode\n");
    } else if(cam_mode == CAM_MODE_HTTPSTREAM) {
        printf("Camera in http stream mode\n");
    } else if(cam_mode == CAM_MODE_UDPSTREAM) {
        printf("Camera in udp stream mode\n");
    }
    /*camera_fb_t* frame_buf = esp_camera_fb_get();
    if(frame_buf) { //This is because the first picture taken after a size change fails.
        esp_camera_fb_return(frame_buf);   
    }*/
}

esp_err_t camera_set_size(int size) {
    /*printf("Camera size %i\n", size);
    stream_frame_size = size;
    if(cam_mode == CAM_MODE_STREAM) {
        sensor->set_framesize(sensor, size);
    }*/
    return 0;
}

esp_err_t camera_set_flip(int f) {
    sensor->set_vflip(sensor, f);  
    sensor->set_hmirror(sensor, f); 
    return 0; 
}

esp_err_t camera_set_auto(int a) {
    if(a != 1) a = 0;
    sensor->set_exposure_ctrl(sensor, a);
    sensor->set_aec2(sensor, a);
    sensor->set_gain_ctrl(sensor, a);
    return 0; 
}

esp_err_t camera_set_gain(int g) {
    sensor->set_gain_ctrl(sensor, 0);
    sensor->set_agc_gain(sensor, g);
    return 0; 
}

esp_err_t camera_set_ae(int l) {
    sensor->set_exposure_ctrl(sensor, 0);
    sensor->set_aec2(sensor, 0);
    sensor->set_ae_level(sensor, l);
    return 0; 
}

esp_err_t camera_set_aec(int l) {
    sensor->set_exposure_ctrl(sensor, 0);
    sensor->set_aec2(sensor, 0);
    sensor->set_aec_value(sensor, l);
    return 0; 
}

esp_err_t camera_set_brightness(int b) {
    sensor->set_brightness(sensor, b);
    return 0; 
}

cam_mode_t camera_get_mode() {
    return cam_mode;
}