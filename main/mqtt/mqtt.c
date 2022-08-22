#include "mqtt.h"

static const char* TAG = "MQTT";

static char COMMAND_SEND_TOPIC[100];
static char COMMAND_RESULT_TOPIC[100];

void mqtt_init() {
    ESP_LOGI(TAG, "Initializing");
    // Generate topic names
    prefix_topic("/commands/send",   &COMMAND_SEND_TOPIC);
    prefix_topic("/commands/result", &COMMAND_RESULT_TOPIC);
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
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            // On successful connection, subscribe to all the topics we want
            esp_mqtt_client_subscribe(client, &COMMAND_SEND_TOPIC, 0);
            break;
        case MQTT_EVENT_DATA:
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            //esp_mqtt_client_publish(client, &COMMAND_RESULT_TOPIC, event->data, event->data_len, 0, 0);
            break;
        default: break;
    }
}

void prefix_topic(const char* topic, char* dst) {
    // Prefix the topic
    char prefixed_topic[100] = "uam-thing/";
    strcat(prefixed_topic, UAM_THING_ID);
    strcat(prefixed_topic, topic);
    // Copy into destination
    strcpy(dst, prefixed_topic);
}