#include "include/monitor.h"
#include "include/camera.h"
#include "include/general.h"
#include "include/config.h"

#define BMP_WIDTH 100
#define BMP_HEIGHT 75
#define BMP_SIZE (BMP_WIDTH * BMP_HEIGHT)

monitor_handler_t monitor_handler;
uint8_t * ref_bmp;
uint8_t * dif_bmp;
uint8_t * cur_bmp;
uint8_t luminosity;
uint8_t settling;

bool is_area_on(int i) {
    uint8_t x = (i % BMP_WIDTH);
    uint8_t y = (i / BMP_WIDTH);
    uint8_t mx = 10 * x / BMP_WIDTH;
    uint8_t my = 10 * y / BMP_HEIGHT;
    uint16_t mbit = (my * 10) + mx;
    uint8_t mbyte = mbit / 8;
    uint8_t msubbit = mbit % 8;
    uint8_t res = threshmap[mbyte] & (0x01 << (7 - msubbit));
    return res > 0;
}

void loop() {
    while(true) {
        if(camera_get_mode() == CAM_MODE_MONITOR) {
            camera_fb_t* frame_buf = esp_camera_fb_get();
            if(frame_buf && frame_buf->format == PIXFORMAT_JPEG) {
                uint8_t * bmp = malloc(2 * BMP_SIZE); 
                bool bmpsuccess = jpg2rgb565(frame_buf->buf, frame_buf->len, bmp, JPG_SCALE_8X); 
                if(bmpsuccess == true) {
                    uint32_t sum_pix = 0;
                    uint32_t sum_diff = 0;
                    uint32_t sum_on = 0;
                    for(int i = 0; i < BMP_SIZE; i++) {
                        if(is_area_on(i)) {
                            uint16_t cpixel = ((uint16_t)bmp[(i*2)+1] * 256) + (uint16_t)bmp[(i*2)+0];
                            uint16_t red = ((cpixel & 0xF800)>>11);
                            uint16_t green = ((cpixel & 0x07E0)>>5);
                            uint16_t blue = (cpixel & 0x001F);
                            uint8_t gpixel = (uint8_t)(((2 * red) + (7 * green) + (1 * blue)) / 10);
                            cur_bmp[i] = gpixel;
                            uint8_t old_gpixel = ref_bmp[i];
                            uint8_t diff = abs(gpixel - old_gpixel);
                            dif_bmp[i] = diff;
                            uint8_t new_ref_gpixel = (0.3 * gpixel) + (0.7 * old_gpixel);
                            ref_bmp[i] = new_ref_gpixel;
                            sum_pix += (uint32_t)gpixel;                            
                            sum_on += 1;
                            if(diff > hpthresh) {
                                sum_diff += (uint32_t)diff;
                            }
                        } else {
                            cur_bmp[i] = 0;
                            dif_bmp[i] = 0;
                            ref_bmp[i] = 0;
                        }
                    }
                    luminosity = (sum_pix / BMP_SIZE); //(uint8_t)(sum_cur / BMP_SIZE);
                    uint16_t diff = (uint16_t)(100 * sum_diff / sum_on);
                    printf("Monitoring sum_on=%lu, luminosity=%u, settling=%u, diff=%u\n", sum_on, luminosity, settling, diff);
                    if(diff > threshold && settling == 0) {
                        monitor_handler(diff);
                        settling = 20;
                    }                    
                    if(settling > 0) settling--;
                }
                free(bmp);
            } else {
                printf("Cannot monitor: %i %i %i\n", frame_buf->format, frame_buf->width, frame_buf->height);
            }
            esp_camera_fb_return(frame_buf);   
        }
        vTaskDelay(delay500ms);
    }
}

void init_monitor(monitor_handler_t handler) {
    monitor_handler = handler;
    ref_bmp = malloc(BMP_SIZE); 
    dif_bmp = malloc(BMP_SIZE);
    cur_bmp = malloc(BMP_SIZE);
    luminosity = 0;
    settling = 30;
    TaskHandle_t xHandle = NULL;
    static uint8_t ucParameterToPass;
    xTaskCreate( loop, "monitor_loop", 2000, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
}


uint8_t* monitor_get_cur_bmp() {
    return cur_bmp;
}

uint8_t* monitor_get_ref_bmp() {
    return ref_bmp;
}

uint8_t* monitor_get_dif_bmp() {
    return dif_bmp;
}

uint8_t monitor_get_luminosity() {
    return luminosity;
}

void monitor_reset_settling() {
    settling = 10;
}