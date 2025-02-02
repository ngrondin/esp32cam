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
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "sdkconfig.h"
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

#define FILTER_PIN_1 GPIO_NUM_13
#define FILTER_PIN_2 GPIO_NUM_14

#define IR_PIN GPIO_NUM_2
#define LED_PIN GPIO_NUM_4

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
const TickType_t delay100ms = 100 / portTICK_PERIOD_MS;

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

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

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    //.frame_size = FRAMESIZE_SVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    .frame_size = FRAMESIZE_VGA,

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    //.fb_location = CAMERA_FB_IN_DRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

const char *ssid = "nicnet";
const char *pass = "nicogrondin01";
int retry_num=3;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data) {
    if(event_id == WIFI_EVENT_STA_START) {
        printf("WIFI connecting....\n");
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi connected\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi lost connection\n");
        if(retry_num<5){
            esp_wifi_connect();retry_num++;printf("WiFi retrying to connect...\n");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("Wifi got IP...\n\n");
    }
}

void wifi_connection() {
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
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",ssid,pass);
}



static esp_err_t api_post_handler(httpd_req_t *req) {
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

    cJSON *root = cJSON_Parse(buf);
    cJSON *ir = cJSON_GetObjectItem(root, "ir");
    if(ir != 0) {
        char *irState = ir->valuestring;
        if(strcmp(irState, "on") == 0) {
            printf("IR on\n");
            gpio_set_level(IR_PIN, 1);
        } else if(strcmp(irState, "off") == 0) {
            printf("IR off\n");
            gpio_set_level(IR_PIN, 0);
        }
    }

    cJSON *led = cJSON_GetObjectItem(root, "led");
    if(led != 0) {
        char *ledState = led->valuestring;
        if(strcmp(ledState, "on") == 0) {
            printf("LED on\n");
            gpio_set_level(LED_PIN, 1);
        } else if(strcmp(ledState, "off") == 0) {
            printf("LED off\n");
            gpio_set_level(LED_PIN, 0);
        }
    }

    cJSON *filter = cJSON_GetObjectItem(root, "filter");
    if(filter != 0) {
        char *filterState = filter->valuestring;
        if(strcmp(filterState, "on") == 0) {
            printf("Filter on\n");
            gpio_set_level(FILTER_PIN_1, 1);
            vTaskDelay(delay100ms);
            gpio_set_level(FILTER_PIN_1, 0);
        } else if(strcmp(filterState, "off") == 0) {
            printf("Filter off\n");
            gpio_set_level(FILTER_PIN_2, 1);
            vTaskDelay(delay100ms);
            gpio_set_level(FILTER_PIN_2, 0);        
        }
    }

    cJSON *flip = cJSON_GetObjectItem(root, "flip");
    if(flip != 0) {
        char *flipState = flip->valuestring;
        if(strcmp(flipState, "on") == 0) {
            printf("Flip on\n");
            sensor_t * s = esp_camera_sensor_get();
            s->set_vflip(s, 1);
        } else if(strcmp(flipState, "off") == 0) {
            printf("Flip off\n");
            sensor_t * s = esp_camera_sensor_get();
            s->set_vflip(s, 0);     
        }
    }

    cJSON *size = cJSON_GetObjectItem(root, "size");
    if(size != 0) {
        char *sizeState = size->valuestring;
        int val = atoi(sizeState);
        sensor_t * s = esp_camera_sensor_get();
        s->set_framesize(s, val);
    }

    cJSON_Delete(root);
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

static esp_err_t api_options_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "*");
    httpd_resp_sendstr(req, "ok");
    printf("Options request\n");
    return ESP_OK;
}

static esp_err_t picture_get_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    struct timeval tv_now;

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        gettimeofday(&tv_now, NULL);
        uint64_t start = (uint64_t)tv_now.tv_sec * 1000000L + (uint64_t)tv_now.tv_usec;
        fb = esp_camera_fb_get();
        if (!fb) {
            printf("Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                printf("JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        gettimeofday(&tv_now, NULL);
        uint64_t end = (uint64_t)tv_now.tv_sec * 1000000L + (uint64_t)tv_now.tv_usec;
        uint64_t dur = (end - start) / 1000L;
        if(dur < 100L) {
            TickType_t sleep_dur = (100L - dur) / portTICK_PERIOD_MS;
            //printf("Dur %llu ms, Sleep %lu ticks\n", dur, sleep_dur);
            vTaskDelay(sleep_dur);
        }
    }

    return res;
}

static const httpd_uri_t api = {
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

static const httpd_uri_t picture = {
    .uri       = "/picture",
    .method    = HTTP_GET,
    .handler   = picture_get_handler,
    .user_ctx  = "Hello World!"
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    printf("Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        
        printf("Registering URI handlers");
        httpd_register_uri_handler(server, &api);
        httpd_register_uri_handler(server, &api_option);
        httpd_register_uri_handler(server, &picture);
        return server;
    }

    printf("Error starting server!");
    return NULL;
}


static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        printf("Camera Init Failed");
        return err;
    }
    return ESP_OK;
}

static esp_err_t init_gpio() 
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

void app_main(void)
{
    nvs_flash_init();
    wifi_connection(); 
    start_webserver();  
    init_camera();
    init_gpio();
    gpio_dump_io_configuration(stdout, (1ULL << IR_PIN) | (1ULL << LED_PIN) | (1ULL << FILTER_PIN_1) | (1ULL << FILTER_PIN_2));
}