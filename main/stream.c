#include "include/stream.h"
#include "include/camera.h"
#include "include/general.h"
#include "include/config.h"
#include <lwip/sockets.h>
#include "include/tools.h"

int udp_sock;
struct sockaddr_in addr;
uint8_t stream_id;

void stream_loop() {
    uint8_t img_num = 0;
    uint8_t packet_buf[520];
    tool_timer_t timer;
    while(true) {
        timer_start(&timer);
        if(camera_get_mode() == CAM_MODE_UDPSTREAM) {
            camera_fb_t* frame_buf = esp_camera_fb_get();
            packet_buf[0] = stream_id;
            packet_buf[1] = img_num;
            if(frame_buf && frame_buf->format == PIXFORMAT_JPEG) {
                uint16_t packet_num = 0;
                for(uint16_t i = 0; i < frame_buf->len; i += 500) {
                    size_t chunk_len = frame_buf->len - i < 500 ? frame_buf->len - i : 500;
                    packet_buf[2] = (packet_num >> 8) & 0xFF;
                    packet_buf[3] = packet_num & 0xFF;
                    for(uint16_t j = 0; j < chunk_len; j++) {
                        packet_buf[4 + j] = frame_buf->buf[i + j];
                    }
                    int ret = sendto(udp_sock, packet_buf, chunk_len + 4, 0, (struct sockaddr *)&addr, sizeof(addr));
                    if(ret == -1) {
                        printf("Error sending UDP %i\n", errno); 
                        break;                   
                    }
                    packet_num++;
                }
            }
            esp_camera_fb_return(frame_buf); 
            img_num++;  
        }
        timer_end(&timer);
        if(timer.dur < 100L) {
            vTaskDelay((100L - timer.dur) / portTICK_PERIOD_MS);
        }  
    }
}

void init_stream() {
    TaskHandle_t  xHandle = NULL;
    static uint8_t ucParameterToPass;
    xTaskCreate( stream_loop, "stream_loop", 10000, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
}

int stream_start(char* ip, uint8_t id) {
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock < 0) {
        return -1;
    }
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10999);
    addr.sin_addr.s_addr = inet_addr(ip);
    stream_id = id;
    camera_set_mode(CAM_MODE_UDPSTREAM);
    return 0;
}

void stream_stop() {
    camera_set_mode(CAM_MODE_MONITOR);
    close(udp_sock);
    udp_sock = 0;
}