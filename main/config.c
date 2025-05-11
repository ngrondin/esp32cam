#include "include/config.h"
#include <nvs.h>
#include "nvs_flash.h" 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int config_ready = 0;
char* wifi_ssid;
char* wifi_pass;
char* mqtt_uri;
char* mqtt_queue_base;
char* cam_name;
char* ip_str;
char* mqtt_cmd_queue;
char* mqtt_stat_queue;
char* mqtt_move_queue;
uint16_t threshold;
uint16_t hpthresh;
uint8_t* threshmap;

static void get_line(char buf[], size_t len)
{
    //memset(buf, 0, len);
    //fpurge(stdin); //clears any junk in stdin
    char *bufp;
    bufp = buf;
    while(true)
        {
            vTaskDelay(100/portTICK_PERIOD_MS);
            *bufp = getchar();
            if(*bufp != '\0' && *bufp != 0xFF && *bufp != '\r') //ignores null input, 0xFF, CR in CRLF
            {
                if(*bufp == '\n'){
                    *bufp = '\0';
                    break;
                } 
                else if (*bufp == '\b'){
                    if(bufp-buf >= 1)
                        bufp--;
                }
                else{
                    bufp++;
                }
                //putchar(*bufp);
            }
            
            if(bufp-buf > (len)-2){
                bufp = buf + (len -1);
                *bufp = '\0';
                break;
            }
        } 
}

esp_err_t write_config() {
    nvs_handle_t handle;
    printf("Writing config: %s, %s, %s, %s, %s, %u\n", cam_name, wifi_ssid, wifi_pass, mqtt_uri, mqtt_queue_base, threshold);
    esp_err_t err = nvs_open("cfg", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return err;
    }
    err = nvs_set_str(handle, "name", cam_name);
    if(err != ESP_OK) return err;
    err = nvs_set_str(handle, "ssid", wifi_ssid);
    if(err != ESP_OK) return err;
    err = nvs_set_str(handle, "pass", wifi_pass);
    if(err != ESP_OK) return err;
    err = nvs_set_str(handle, "mqtturi", mqtt_uri);
    if(err != ESP_OK) return err;
    err = nvs_set_str(handle, "qbase", mqtt_queue_base);
    if(err != ESP_OK) return err;
    err = nvs_set_blob(handle, "map", threshmap, 15);
    if(err != ESP_OK) return err;
    err = nvs_set_u16(handle, "thresh", threshold);
    if(err != ESP_OK) return err;
    err = nvs_set_u16(handle, "hpthresh", hpthresh);
    if(err != ESP_OK) return err;
    nvs_commit(handle);
    nvs_close(handle);
    printf("Saved config\n");
    return 0;
}

esp_err_t input_config() {
    printf("Name: \n");
    get_line(cam_name, 10);
    printf("Wifi SSID: \n");
    get_line(wifi_ssid, 20);
    printf("Wifi Password: \n");
    get_line(wifi_pass, 30);
    printf("MQTT Uri: \n");
    get_line(mqtt_uri, 50);
    printf("MQTT Queue Base: \n");
    get_line(mqtt_queue_base, 50);
    esp_err_t err = write_config();
    if(err == ESP_OK) {
        config_ready = 1;
        return 0;
    } else {
        return err;
    }
}

esp_err_t read_config() {
    nvs_handle_t handle;
    printf("Reading config\n");
    esp_err_t err = nvs_open("cfg", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return err;
    }
    size_t len = 10;
    err = nvs_get_str(handle, "name", cam_name, &len);
    if(err == ESP_OK) {
        printf("Cam name is: %s\n", cam_name);
    } else {
        printf("Error (%s) reading name!\n", esp_err_to_name(err));
        return err;
    }
    len = 20;
    err = nvs_get_str(handle, "ssid", wifi_ssid, &len);
    if(err == ESP_OK) {
        printf("Wifi ssid is: %s\n", wifi_ssid);
    } else {
        printf("Error (%s) reading wifi ssid!\n", esp_err_to_name(err));
        return err;
    }
    len = 30;
    err = nvs_get_str(handle, "pass", wifi_pass, &len);
    if(err == ESP_OK) {
        printf("Wifi pass has been read\n");
    } else {
        printf("Error (%s) reading wifi pass!\n", esp_err_to_name(err));
        return err;
    }
    len = 50;
    err = nvs_get_str(handle, "mqtturi", mqtt_uri, &len);
    if(err == ESP_OK) {
        printf("Mqtt uri is: %s\n", mqtt_uri);
    } else {
        printf("Error (%s) reading mqtt uri!\n", esp_err_to_name(err));
        return err;
    }
    len = 50;
    err = nvs_get_str(handle, "qbase", mqtt_queue_base, &len);
    if(err == ESP_OK) {
        printf("Mqtt queue base is: %s\n", mqtt_queue_base);
    } else {
        printf("Error (%s) reading mqtt queue base !\n", esp_err_to_name(err));
        return err;
    }
    len = 15;
    err = nvs_get_blob(handle, "map", threshmap, &len);
    if(err != ESP_OK) {
        for(int i = 0; i < 15; i++) threshmap[i] = 255;
    }    
    printf("Threashmap is:");
    for(int i = 0; i < 15; i++) printf(" %u", threshmap[i]);
    printf("\n");
    err = nvs_get_u16(handle, "thresh", &threshold);
    if(err != ESP_OK) {
        threshold = 50;
    }
    printf("Threshold is: %u\n", hpthresh);
    err = nvs_get_u16(handle, "hpthresh", &threshold);
    if(err != ESP_OK) {
        hpthresh = 10;
    }  
    printf("Highpass Threshold is: %u\n", hpthresh);
    
    sprintf(mqtt_cmd_queue, "%s/%s/cmd", mqtt_queue_base, cam_name);
    sprintf(mqtt_stat_queue, "%s/%s/stat", mqtt_queue_base, cam_name);
    sprintf(mqtt_move_queue, "%s/%s/move", mqtt_queue_base, cam_name);
    config_ready = 1;
    nvs_close(handle);
    return 0;
}

esp_err_t init_config() {
    nvs_flash_init();
    cam_name = malloc(10);
    wifi_ssid = malloc(20);
    wifi_pass = malloc(30);
    mqtt_uri = malloc(50);
    mqtt_queue_base = malloc(50);
    mqtt_cmd_queue = malloc(100);
    mqtt_stat_queue = malloc(100);
    mqtt_move_queue = malloc(100);
    ip_str = malloc(17);
    threshold = 50;
    hpthresh = 10;
    threshmap = malloc(15);
    for(int i = 0; i < 15; i++) threshmap[i] = 255;
    config_ready = 0;
    printf("Config initiated\n");
    return 0;
}