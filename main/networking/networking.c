#include "networking.h"

static const char* TAG = "Networking";

void networking_init() {
    ESP_LOGI(TAG, "Initializing");
    // Configure the access point
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t ap_conf = {
        .ap = {
            .ssid = NET_AP_SSID,
            .ssid_len = strlen(NET_AP_SSID),
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 3,
        }
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_conf);
    // Register event handler for both WiFi and IP events
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &networking_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &networking_event_handler, NULL, NULL);
    // Start networking straight away
    networking_start();
}
void networking_start() {
    ESP_ERROR_CHECK(esp_wifi_start());
}

void networking_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        // New station connected and IP assigned, get MQTT URI
        ip4_addr_t* ip = get_last_ip();
        ESP_LOGI(TAG, "New connection, IP: %s", ip4addr_ntoa(ip));
        const char* uri = ip_to_uri("mqtt://", ip, ":1883");
        free(ip);
        // Set URI and start MQTT
        mqtt_uri(uri);
        mqtt_start();
    }
}

const char* ip_to_uri(const char* prefix, ip4_addr_t* ip, const char* suffix) {
    static char uri[100];
    strcat(uri, prefix);
    strcat(uri, ip4addr_ntoa(ip));
    strcat(uri, suffix);
    return &uri;
}
ip4_addr_t* get_last_ip() {
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    //memset(&wifi_sta_list, 0, sizeof(wifi_sta_list)); memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    if(adapter_sta_list.num == 0) return NULL;

    //ip_addr_t* gw_addr = malloc(sizeof(ip_addr_t));
    //memcpy((char *)gw_addr.u_addr.ip4, (char *)&ip_info.gw, sizeof(ip4_addr));
    //gw_addr.type = IPADDR_TYPE_V4;

    ip4_addr_t* addr = malloc(sizeof(ip4_addr_t));
    memcpy((char*)addr, (char*)&adapter_sta_list.sta[0].ip, sizeof(ip4_addr_t));
    ESP_LOGI(TAG, "ip4addr_ntoa(addr)=%s", ip4addr_ntoa(addr));
    return addr;
    
    //esp_ip4_addr_t* addr = malloc(sizeof(esp_ip4_addr_t));
    //memcpy(addr, &adapter_sta_list.sta[0].ip, sizeof(esp_ip4_addr_t));
    //return addr;
}