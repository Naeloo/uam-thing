#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "mqtt/mqtt.h"
#include "networking/networking.h"
#include "sensors/sensors.h"
#include "sensors/pressure/sensors_pressure.h"

static const char* TAG = "Main";

// Component Event Handlers
// These are used to connect between components via event handlers
// to avoid cyclic dependencies
void on_pressure_reading(pressure_sensor* sensor, int reading) {
    //ESP_LOGI(TAG, "Reading: %d", reading);
    mqtt_publish_sensor(sensor_type_pressure, (char*)&sensor->id, reading);
}

// Entry point function
void app_main(void)
{
    // Initialize necessary libraries
    ESP_LOGI(TAG, "Initializing system");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Initialize WiFi
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure components
    sensors_pressure_on_reading(&on_pressure_reading);

    // Initialize components
    ESP_LOGI(TAG, "Initializing components");
    mqtt_init();
    networking_init();
    sensors_pressure_init();
    sensors_pressure_start();
}