idf_component_register(SRCS "main.c"
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    REQUIRES json
                    INCLUDE_DIRS ".")