#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "../mqtt/mqtt.h"
#include "../constants.h"

#define NET_AP_SSID "UAM Thing " UAM_THING_NAME

void networking_init();
void networking_start();

void networking_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data);
const char* ip_to_uri(const char* prefix, ip4_addr_t* ip, const char* suffix);
ip4_addr_t* get_last_ip();