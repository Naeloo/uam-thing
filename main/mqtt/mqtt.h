#include "mqtt_client.h"
#include "esp_log.h"
#include "../constants.h"
#include "../sensors/sensors.h"

esp_mqtt_client_handle_t mqtt_client;

void mqtt_init();
void mqtt_uri(const char* uri);
void mqtt_start();
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_publish_sensor(sensor_type sensor, char* sensor_id, int reading);

void prefix_topic(const char* topic, char* dst);