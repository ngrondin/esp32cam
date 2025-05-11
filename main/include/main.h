#include <esp_http_server.h>

#ifndef MAIN_H
#define MAIN_H


#define PART_BOUNDARY "123456789000000000000987654321"
#define STREAM_CONTENT_TYPE ( "multipart/x-mixed-replace;boundary=" PART_BOUNDARY )
#define STREAM_BOUNDARY ( "\r\n--" PART_BOUNDARY "\r\n" )
#define STREAM_PART ( "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n" )
#define PICTURE_CONTENT_TYPE ( "image/jpeg" )

int retry_num=3;

void init_mqtt(void);
void init_wifi();
httpd_handle_t init_webserver();
esp_err_t init_gpio();

void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
esp_err_t api_post_handler(httpd_req_t *req);
esp_err_t api_options_handler(httpd_req_t *req);
esp_err_t stream_get_handler(httpd_req_t *req);
esp_err_t monitor_ref_stream_get_handler(httpd_req_t *req);
esp_err_t monitor_cur_stream_get_handler(httpd_req_t *req);
esp_err_t monitor_dif_stream_get_handler(httpd_req_t *req);
esp_err_t picture_get_handler(httpd_req_t *req);

#endif