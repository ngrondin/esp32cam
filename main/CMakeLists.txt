idf_component_register(SRCS "stream.c" "monitor.c" "tools.c" "camera.c" "api.c" "config.c" "main.c"
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    REQUIRES json
                    REQUIRES mqtt
                    INCLUDE_DIRS ".")