#include "sensors_pressure.h"

struct pressure_sensor sensors[PRESSURE_SENSORS_COUNT] = {
    { .id = "p1", .channel = ADC1_CHANNEL_6 }
};
TaskHandle_t measureHandle = NULL;
sensors_pressure_handler reading_handler = NULL;

void sensors_pressure_init() {
    // We're only using ADC1 for this
    // Set ADC1 to full bandwidth
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    // Configure all the ADC channels
    for(int i=0; i<PRESSURE_SENSORS_COUNT; i++) {
        // Attenuation 11dB = 150 mV ~ 2450 mV
        // https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/peripherals/adc.html#adc-attenuation
        ESP_ERROR_CHECK(adc1_config_channel_atten(sensors[i].channel, ADC_ATTEN_DB_11));
    }
}
void sensors_pressure_start() {
      xTaskCreate(
        sensors_pressure_measure,
        "PressureSensors",   // Name of the task (for debugging)
        8192,            // Stack size (bytes)
        NULL,            // Parameter to pass
        1,               // Task priority
        &measureHandle             // Task handle
    );
}
void sensors_pressure_stop() {
    vTaskDelete(&measureHandle);
}
void sensors_pressure_measure() {
    while(1) {
        // Get values from all the sensors
        for(int i=0;i<PRESSURE_SENSORS_COUNT;i++) {
            int reading = adc1_get_raw(sensors[i].channel);
            // TODO: Ideally this would be pushed out in a queue and another
            // network bound task would read it and publish, right now the sensor task also publishes
            if(reading_handler != NULL) reading_handler(&sensors[i], reading);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void sensors_pressure_on_reading(sensors_pressure_handler h) {
    reading_handler = h;
}