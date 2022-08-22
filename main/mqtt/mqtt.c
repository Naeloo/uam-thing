#include "mqtt.h"

static const char* TAG = "MQTT";

static char COMMAND_SEND_TOPIC[100];
static char COMMAND_RESULT_TOPIC[100];
static char PRESSURE_TOPIC[100];
static char FLOW_TOPIC[100];

bool is_connected = false;

void mqtt_init() {
    ESP_LOGI(TAG, "Initializing");
    // Generate topic names
    prefix_topic("/commands/send",    (char*)&COMMAND_SEND_TOPIC);
    prefix_topic("/commands/result",  (char*)&COMMAND_RESULT_TOPIC);
    prefix_topic("/sensors/pressure", (char*)&PRESSURE_TOPIC);
    prefix_topic("/sensors/flow",     (char*)&FLOW_TOPIC);
    // Initialize MQTT
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    // Register the event handler
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}
void mqtt_uri(const char* uri) {
    ESP_LOGI(TAG, "MQTT Broker URI: %s", uri);
    esp_mqtt_client_set_uri(mqtt_client, uri);
}
void mqtt_start() {
    ESP_LOGI(TAG, "Starting up");
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            // On successful connection, subscribe to all the topics we want
            esp_mqtt_client_subscribe(client, (const char*)&COMMAND_SEND_TOPIC, 0);
            // Set the global variable that lets everyone know
            is_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            // On disconnect, prevent any further MQTT action
            is_connected = false;
            break;
        case MQTT_EVENT_DATA:
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            //esp_mqtt_client_publish(client, &COMMAND_RESULT_TOPIC, event->data, event->data_len, 0, 0);
            break;
        default: break;
    }
}
void mqtt_publish_sensor(sensor_type sensor, char* sensor_id, int reading) {
    // If we're not connected, skip
    if(!is_connected) return;
    // Determine sensor topic
    char sensor_topic[100];
    switch(sensor) {
        case sensor_type_pressure: strcpy(sensor_topic, PRESSURE_TOPIC); break;
        case sensor_type_flow:     strcpy(sensor_topic, FLOW_TOPIC); break;
        default: return;
    }
    strcat(sensor_topic, "/");
    strcat(sensor_topic, sensor_id);
    //ESP_LOGI(TAG, "%s Sensor Topic: %s", PRESSURE_TOPIC, sensor_topic);
    // Generate sensor message
    char sensor_message[12];
    sprintf(sensor_message, "%d", reading);
    // Publish
    esp_mqtt_client_publish(mqtt_client, (const char*)&sensor_topic, (const char*)&sensor_message, strlen((const char*)&sensor_message), 0, 0);
}

void prefix_topic(const char* topic, char* dst) {
    // Prefix the topic
    char prefixed_topic[100] = "uam-thing/";
    strcat(prefixed_topic, UAM_THING_ID);
    strcat(prefixed_topic, topic);
    // Copy into destination
    strcpy(dst, prefixed_topic);
}