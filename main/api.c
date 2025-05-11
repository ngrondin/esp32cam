#include <string.h> 
#include "cJSON.h"
#include "include/api.h"
#include "include/general.h"
#include "include/camera.h"
#include "include/stream.h"
#include "include/config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void api_handler(char *buf) {
    cJSON *root = cJSON_Parse(buf);
    cJSON *val;

    val = cJSON_GetObjectItem(root, "ir");
    if(val != 0) {
        if(val->valueint == 1) {
            printf("IR on\n");
            gpio_set_level(IR_PIN, 1);
        } else {
            printf("IR off\n");
            gpio_set_level(IR_PIN, 0);
        }
    }

    val = cJSON_GetObjectItem(root, "led");
    if(val != 0) {
        if(val->valueint == 1) {
            printf("LED on\n");
            gpio_set_level(LED_PIN, 1);
        } else {
            printf("LED off\n");
            gpio_set_level(LED_PIN, 0);
        }
    }

    val = cJSON_GetObjectItem(root, "filter");
    if(val != 0) {
        if(val->valueint == 1) {
            printf("Filter on\n");
            gpio_set_level(FILTER_PIN_1, 1);
            vTaskDelay(delay100ms);
            gpio_set_level(FILTER_PIN_1, 0);
        } else {
            printf("Filter off\n");
            gpio_set_level(FILTER_PIN_2, 1);
            vTaskDelay(delay100ms);
            gpio_set_level(FILTER_PIN_2, 0);        
        }
    }

    val = cJSON_GetObjectItem(root, "flip");
    if(val != 0) {
        if(val->valueint == 1) {
            printf("Flip on\n");
            camera_set_flip(1);
        } else {
            printf("Flip off\n");
            camera_set_flip(0);
        }
    }

    val = cJSON_GetObjectItem(root, "size");
    if(val != 0) {
        camera_set_size(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "auto");
    if(val != 0) {
        printf("Auto: %i\n", val->valueint);
        camera_set_auto(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "gain");
    if(val != 0) {
        printf("Gain: %i\n", val->valueint);
        camera_set_gain(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "ae");
    if(val != 0) {
        printf("AE: %i\n", val->valueint);
        camera_set_ae(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "aec");
    if(val != 0) {
        printf("AEC: %i\n", val->valueint);
        camera_set_aec(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "brightness");
    if(val != 0) {
        printf("Brightness: %i\n", val->valueint);
        camera_set_brightness(val->valueint);
    }

    val = cJSON_GetObjectItem(root, "reboot");
    if(val != 0) {
        printf("Reboot: %i\n", val->valueint);
        esp_restart();
    }

    val = cJSON_GetObjectItem(root, "threshold");
    if(val != 0) {
        printf("Threshold: %i\n", val->valueint);
        threshold = val->valueint;
        write_config();
    }

    val = cJSON_GetObjectItem(root, "hpthresh");
    if(val != 0) {
        printf("Highpass Threshold: %i\n", val->valueint);
        hpthresh = val->valueint;
        write_config();
    }

    val = cJSON_GetObjectItem(root, "threshmap");
    if(val != 0) {
        for(int i = 0; i < 15; i++) threshmap[i] = 255;
        val = val->child;
        int i = 0;
        while(val != NULL) {
            threshmap[i] = val->valueint;
            val = val->next;
            i++;
        }
        printf("Threashmap:");
        for(int i = 0; i < 15; i++) printf(" %u", threshmap[i]);
        printf("\n");
        write_config();
    }

    val = cJSON_GetObjectItem(root, "streamto");
    if(val != 0) {
        if(strlen(val->valuestring) > 0) {
            cJSON *id_val = cJSON_GetObjectItem(root, "streamid");
            printf("Stream to: %s, id:%i\n", val->valuestring, id_val->valueint);
            stream_start(val->valuestring, id_val->valueint);
        } else {
            printf("Stream stop\n");
            stream_stop();
        }
    }

    cJSON_Delete(root);
}