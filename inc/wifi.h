#ifndef WIFI_H_
#define WIFI_H_
//------------------------------------------------
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdbool.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/inet.h"
#include <lwip/netdb.h>
#include "user_config.h"
#include "gpio.h"
//------------------------------------------------
typedef void (* wifi_state_cb_t)();
typedef void (* wifi_disco_cb_t)(uint8 reason);
bool ICACHE_FLASH_ATTR start_wifi_station(const char * ssid, const char * pass);
bool ICACHE_FLASH_ATTR start_wifi_ap(const char * ssid, const char * pass);
//------------------------------------------------
#endif /* WIFI_H_ */
