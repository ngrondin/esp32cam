#include <stdio.h> //for basic printf commands
#include <stdlib.h>
#include <string.h> //for handling strings
#include <unistd.h>
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"
#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_log.h" //for showing logs
#include "esp_event.h" //for wifi event
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps
#include "esp_http_server.h"
#include "esp_vfs.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include <soc/gpio_periph.h>
#include "mqtt_client.h"
#include "cJSON.h"

#include "include/main.h"
#include "include/general.h"
#include "include/config.h"
#include "include/api.h"
#include "include/camera.h"
#include "include/tools.h"
#include "include/monitor.h"
#include "include/stream.h"


#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

esp_mqtt_client_handle_t mqtt_client;

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;



static const httpd_uri_t api_post = {
    .uri       = "/api",
    .method    = HTTP_POST,
    .handler   = api_post_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t api_option = {
    .uri       = "/api",
    .method    = HTTP_OPTIONS,
    .handler   = api_options_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t stream = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_get_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t monitor_ref = {
    .uri       = "/monitor/ref",
    .method    = HTTP_GET,
    .handler   = monitor_ref_stream_get_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t monitor_cur = {
    .uri       = "/monitor/cur",
    .method    = HTTP_GET,
    .handler   = monitor_cur_stream_get_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t monitor_dif = {
    .uri       = "/monitor/dif",
    .method    = HTTP_GET,
    .handler   = monitor_dif_stream_get_handler,
    .user_ctx  = "Hello World!"
};

static const httpd_uri_t picture = {
    .uri       = "/picture",
    .method    = HTTP_GET,
    .handler   = picture_get_handler,
    .user_ctx  = "Hello World!"
};

void send_status() {
    if(mqtt_client) {
        uint8_t lum = monitor_get_luminosity();
        cJSON *msg_json = cJSON_CreateObject();
        cJSON_AddItemToObject(msg_json, "ip", cJSON_CreateString(ip_str));
        cJSON_AddItemToObject(msg_json, "lum", cJSON_CreateNumber((double)lum));
        char *msg = cJSON_Print(msg_json);
        cJSON_Delete(msg_json);
        esp_mqtt_client_publish(mqtt_client, mqtt_stat_queue, msg, 0, 0, 0);
        free(msg);    
    }
}



void init_wifi() {
    esp_netif_init();
    esp_event_loop_create_default();    
    esp_netif_create_default_wifi_sta(); 
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
        }
    };
    strcpy((char*)wifi_configuration.sta.ssid, wifi_ssid);
    strcpy((char*)wifi_configuration.sta.password, wifi_pass);    
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
    //printf( "wifi_init_softap finished. SSID:%s  password:%s", wifi_ssid, wifi_pass);
}

void init_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    printf("MQTT Started\n");
}

httpd_handle_t init_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    printf("Starting server on port: '%d'\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        
        printf("Registering URI handlers\n");
        httpd_register_uri_handler(server, &api_post);
        httpd_register_uri_handler(server, &api_option);
        httpd_register_uri_handler(server, &picture);
        httpd_register_uri_handler(server, &stream);
        httpd_register_uri_handler(server, &monitor_ref);
        httpd_register_uri_handler(server, &monitor_cur);
        httpd_register_uri_handler(server, &monitor_dif);
        return server;
    }

    printf("Error starting server!");
    return NULL;
}

esp_err_t init_gpio() 
{
    ESP_ERROR_CHECK( gpio_set_direction(FILTER_PIN_1, GPIO_MODE_OUTPUT) );
    ESP_ERROR_CHECK( gpio_hold_dis(FILTER_PIN_1) );
    ESP_ERROR_CHECK( gpio_pullup_dis(FILTER_PIN_1) );
    ESP_ERROR_CHECK( gpio_pulldown_dis(FILTER_PIN_1) );
    ESP_ERROR_CHECK( gpio_set_level(FILTER_PIN_1, 0) );
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[FILTER_PIN_1], PIN_FUNC_GPIO); //This is important to disable JTAG debugging
    
    ESP_ERROR_CHECK( gpio_set_direction(FILTER_PIN_2, GPIO_MODE_OUTPUT) );
    ESP_ERROR_CHECK( gpio_pullup_dis(FILTER_PIN_2) );
    ESP_ERROR_CHECK( gpio_pulldown_dis(FILTER_PIN_2) );
    ESP_ERROR_CHECK( gpio_hold_dis(FILTER_PIN_2) );
    ESP_ERROR_CHECK( gpio_set_level(FILTER_PIN_2, 0) );
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[FILTER_PIN_2], PIN_FUNC_GPIO);

    
    ESP_ERROR_CHECK( gpio_set_direction(IR_PIN, GPIO_MODE_OUTPUT) );
    ESP_ERROR_CHECK( gpio_set_level(IR_PIN, 0) );
    
    ESP_ERROR_CHECK( gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT) );
    ESP_ERROR_CHECK( gpio_set_level(LED_PIN, 0) );    

    return ESP_OK;
}



void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_id == WIFI_EVENT_STA_START) {
        printf("WIFI connecting\n");
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi connected\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi lost connection\n");
        if(retry_num < 5){
            esp_wifi_connect();
            retry_num++;
            printf("WiFi retrying to connect\n");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* data = (ip_event_got_ip_t*) event_data;
        sprintf(ip_str,"%i.%i.%i.%i", IP2STR(&data->ip_info.ip));
        printf("Wifi got IP: %s\n", ip_str);
        init_mqtt();
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        printf("MQTT Connected\n");
        esp_mqtt_client_subscribe(client, mqtt_cmd_queue, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("MQTT Subscribed\n");
        send_status();
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        printf("MQTT Data\n");
        char *buf = malloc(event->data_len + 1);
        strncpy(buf, event->data, event->data_len);
        api_handler(buf);
        free(buf);
        break;
    case MQTT_EVENT_ERROR:
        break;
    default:
        break;
    }
}

void cron_event_handler() {
    printf("Cron\n");
    send_status();
}

void monitor_event_handler(uint8_t diff) {
    if(mqtt_client) {
        cJSON *msg_json = cJSON_CreateObject();
        cJSON_AddItemToObject(msg_json, "diff", cJSON_CreateNumber(diff));
        char *msg = cJSON_Print(msg_json);
        cJSON_Delete(msg_json);
        esp_mqtt_client_publish(mqtt_client, mqtt_move_queue, msg, 0, 0, 0);
        free(msg);    
    }

    printf("Monitor diff %i\n", diff);
}

esp_err_t api_post_handler(httpd_req_t *req) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = malloc(total_len + 1);
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    api_handler(buf);
    free(buf);

    cJSON *respRoot = cJSON_CreateObject();
    cJSON_AddItemToObject(respRoot, "response", cJSON_CreateString("OK"));
    char *out = cJSON_Print(respRoot);
    cJSON_Delete(respRoot);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "*");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, out, HTTPD_RESP_USE_STRLEN);
    free(out);
    return ESP_OK;
}

esp_err_t api_options_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "*");
    httpd_resp_sendstr(req, "ok");
    printf("Options request\n");
    return ESP_OK;
}

esp_err_t stream_get_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    char * part_buf[64];
    tool_timer_t timer;

    printf("Starting streaming\n");
    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    camera_set_mode(CAM_MODE_HTTPSTREAM);

    while(true) {
        timer_start(&timer);
        camera_fb_t* frame_buf = esp_camera_fb_get();
        if(frame_buf) {
            if(frame_buf->format == PIXFORMAT_JPEG) {
                if(res == ESP_OK){
                    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
                }
                if(res == ESP_OK){
                    size_t hlen = snprintf((char *)part_buf, 64, STREAM_PART, frame_buf->len);
                    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
                }
                if(res == ESP_OK){
                    res = httpd_resp_send_chunk(req, (const char *)frame_buf->buf, frame_buf->len);
                }    
            }
            esp_camera_fb_return(frame_buf);
        } else {
            res = -1;
        }
        if(res != ESP_OK){
            httpd_resp_send_500(req);
            break;
        } else {
            timer_end(&timer);
            if(timer.dur < 100L) {
                TickType_t sleep_dur = (100L - timer.dur) / portTICK_PERIOD_MS;
                vTaskDelay(sleep_dur);
            }    
        }        
    }
    monitor_reset_settling();
    camera_set_mode(CAM_MODE_MONITOR);
    printf("Stopped streaming\n");
    return res;
}

esp_err_t monitor_stream_get_handler(httpd_req_t *req, uint8_t* (* getter)()) {
    esp_err_t res = ESP_OK;
    char * part_buf[64];
    printf("Starting streaming monitor\n");

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;

    while(true) {
        uint8_t* bmp = getter();
        uint8_t* jpg;
        size_t jpg_len;
        bool jpgsuccess = fmt2jpg(bmp, 100 * 75, 100, 75, PIXFORMAT_GRAYSCALE, 50, &jpg, &jpg_len);
        if(jpgsuccess) {
            if(res == ESP_OK) {
                res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
            }
            if(res == ESP_OK) {
                size_t hlen = snprintf((char *)part_buf, 64, STREAM_PART, jpg_len);
                res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
            }
            if(res == ESP_OK) { 
                res = httpd_resp_send_chunk(req, (const char *)jpg, jpg_len);
            }            
            free(jpg);
        }
        if(!jpgsuccess || res != ESP_OK) {
            httpd_resp_send_500(req);
            break;
        } else {
            TickType_t sleep_dur = 500L / portTICK_PERIOD_MS;
            vTaskDelay(sleep_dur);    
        }
    }
    printf("Finished streaming monitor\n");
    return res;
}

esp_err_t monitor_ref_stream_get_handler(httpd_req_t *req) {
    return monitor_stream_get_handler(req, monitor_get_ref_bmp);
}

esp_err_t monitor_cur_stream_get_handler(httpd_req_t *req) {
    return monitor_stream_get_handler(req, monitor_get_cur_bmp);
}

esp_err_t monitor_dif_stream_get_handler(httpd_req_t *req) {
    return monitor_stream_get_handler(req, monitor_get_dif_bmp);
}

esp_err_t picture_get_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    camera_fb_t* frame_buf;

    printf("Getting picture\n");
    res = httpd_resp_set_type(req, PICTURE_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    camera_set_mode(CAM_MODE_PICTURE);
    frame_buf = esp_camera_fb_get();
    if(frame_buf) {
        if(frame_buf->format == PIXFORMAT_JPEG) {
            if(res == ESP_OK){
                res = httpd_resp_send(req, (const char *)frame_buf->buf, frame_buf->len);
            }    
        }
        esp_camera_fb_return(frame_buf);
    } else {
        httpd_resp_send_500(req);
    }
    monitor_reset_settling();
    camera_set_mode(CAM_MODE_MONITOR);
    printf("Finished sending picture\n");
    return res;
}

void app_main(void)
{
    init_config();
    while(read_config() != ESP_OK) {
        input_config();
    }
    init_gpio();
    init_wifi(); 
    init_webserver();  
    init_cron(cron_event_handler);
    init_camera();
    init_monitor(monitor_event_handler);
    init_stream();
}