#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define PRESSURE_SENSORS_COUNT 1

typedef struct pressure_sensor {
    char id[3];
    adc1_channel_t channel;
} pressure_sensor;

typedef void (*sensors_pressure_handler)(pressure_sensor* sensor, int reading);

void sensors_pressure_init();
void sensors_pressure_start();
void sensors_pressure_measure();

void sensors_pressure_on_reading(sensors_pressure_handler h);