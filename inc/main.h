#ifndef MAIN_H_
#define MAIN_H_
//------------------------------------------------
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "uart.h"
#include "gpio.h"
#include "wifi.h"
//------------------------------------------------
WIFI_MODE ICACHE_FLASH_ATTR init_esp_wifi();
//------------------------------------------------
#endif /* MAIN_H_ */
