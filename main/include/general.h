#include "driver/gpio.h"
#include <soc/gpio_periph.h>
#include "freertos/FreeRTOS.h" 


#ifndef GEN_H
#define GEN_H

#define FILTER_PIN_1 GPIO_NUM_13
#define FILTER_PIN_2 GPIO_NUM_14

#define IR_PIN GPIO_NUM_2
#define LED_PIN GPIO_NUM_4

#define delay100ms ( 100 / portTICK_PERIOD_MS )
#define delay500ms ( 500 / portTICK_PERIOD_MS )
#define delay1000ms ( 1000 / portTICK_PERIOD_MS )
#define delay120000ms ( 120000 / portTICK_PERIOD_MS )

#endif