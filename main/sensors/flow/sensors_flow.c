#include "sensors_flow.h"

static const char* TAG = "SensorsFlow";

struct flow_sensor flow_sensors[FLOW_SENSORS_COUNT] = {
    { .id = "q1", .channel = 0, .serial = FLOW_SERIAL_INVALID, .offset = FLOW_SERIAL_INVALID, .scale = FLOW_SERIAL_INVALID },
    { .id = "q2", .channel = 1, .serial = FLOW_SERIAL_INVALID, .offset = FLOW_SERIAL_INVALID, .scale = FLOW_SERIAL_INVALID },
    { .id = "q3", .channel = 2, .serial = FLOW_SERIAL_INVALID, .offset = FLOW_SERIAL_INVALID, .scale = FLOW_SERIAL_INVALID },
    { .id = "q4", .channel = 3, .serial = FLOW_SERIAL_INVALID, .offset = FLOW_SERIAL_INVALID, .scale = FLOW_SERIAL_INVALID }
};
TaskHandle_t flow_measure_handle = NULL;
sensors_flow_handler flow_reading_handler = NULL;

void sensors_flow_init() {
    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
    // Read sensor serials, determine availability
    for(int i = 0; i < FLOW_SENSORS_COUNT; i++) {
        tca_select_channel(flow_sensors[i].channel);
        flow_sensors[i].serial = sfm_read_serial();
        flow_sensors[i].offset = sfm_read_offset();
        flow_sensors[i].scale = sfm_read_scale();
        if(flow_sensors[i].serial != FLOW_SERIAL_INVALID && flow_sensors[i].offset != FLOW_SERIAL_INVALID && flow_sensors[i].scale != FLOW_SERIAL_INVALID) {
            ESP_LOGI(TAG, "SFM Sensor %s available on channel %u, Serial: %u, Offset: %u, Scale: %u", flow_sensors[i].id, flow_sensors[i].channel, flow_sensors[i].serial, flow_sensors[i].offset, flow_sensors[i].scale);
        }else{
            ESP_LOGI(TAG, "SFM Sensors %s unavailable on channel %u", flow_sensors[i].id, flow_sensors[i].channel);
        }
    }
}

void sensors_flow_start() {
    // Instruct available sensors to start measuring
    // Soft reset all sensors
    for(int i = 0; i < FLOW_SENSORS_COUNT; i++) {
        if(flow_sensors[i].serial == FLOW_SERIAL_INVALID) continue;
        tca_select_channel(flow_sensors[i].channel);
        sfm_start_measure();
    }
    // Create the periodic measuring task
    xTaskCreate(
        sensors_flow_measure,
        "FlowSensors",   // Name of the task (for debugging)
        8192,            // Stack size (bytes)
        NULL,            // Parameter to pass
        1,               // Task priority
        &flow_measure_handle             // Task handle
    );
}
void sensors_flow_stop() {
    // Stop the measurement task
    vTaskDelete(&flow_measure_handle);
    // Soft reset all sensors
    for(int i = 0; i < FLOW_SENSORS_COUNT; i++) {
        if(flow_sensors[i].serial == FLOW_SERIAL_INVALID) continue;
        tca_select_channel(flow_sensors[i].channel);
        sfm_reset();
    }
}
void sensors_flow_measure() {
    while(1) {
        // Get values from all the sensors
        for(int i = 0; i < FLOW_SENSORS_COUNT; i++) {
            // Skip unconnected sensors (no serial)
            if(flow_sensors[i].serial == FLOW_SERIAL_INVALID) continue;
            // Select channel of sensor and read value
            tca_select_channel(flow_sensors[i].channel);
            uint16_t value = sfm_read_measure();
            if(value == FLOW_VALUE_INVALID_CRC) {
                ESP_LOGW(TAG, "Skipped flow value: incorrect CRC");
                continue;
            }
            if(value == FLOW_VALUE_NOT_AVAIL) {
                ESP_LOGW(TAG, "Skipped flow value: no data available / read error");
            }
            float value_slm = (value - flow_sensors[i].offset) / flow_sensors[i].scale;
            ESP_LOGI(TAG, "%.2f SLM", value_slm);
        }
        // Wait for next readings
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void sensors_flow_on_reading(sensors_flow_handler h) {
    flow_reading_handler = h;
}

// TCA Commands
void tca_select_channel(uint8_t channel) {
    // Sanity check the channel number (8 channels available, 0-7)
    if(channel > 7) return;
    // Write the channel config to the TCA
    uint8_t message[] = { 0x01 << channel };
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, I2C_TCA_ADDRESS, message, sizeof(message), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}
// SFM Commands
uint32_t sfm_read_serial() {
    // Set the SFMs internal pointer to serial number
    uint8_t serial_cmd[] = { 0x31, 0xAE };
    esp_err_t write_err = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_cmd, sizeof(serial_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(write_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Read the 4 byte serial number + 2 CRC bytes [serial, serial, crc, serial, serial, crc]
    uint8_t serial_buf[6];
    esp_err_t read_err = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_buf, sizeof(serial_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(read_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Verify both CRC values
    if(sfm_crc(&serial_buf[0], 2, serial_buf[2]) != ESP_OK || sfm_crc(&serial_buf[3], 2, serial_buf[5]) != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Convert to 32 bit number and return
    uint32_t serial_no = serial_buf[4] | serial_buf[3] << 8 | serial_buf[1] << 16 | serial_buf[0] << 24;
    return serial_no;
}
uint16_t sfm_read_offset() {
    uint8_t offset_cmd[] = { 0x30, 0xDF };
    esp_err_t write_err = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, offset_cmd, sizeof(offset_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(write_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Read the 2 byte flow offset value
    uint8_t offset_buf[2];
    esp_err_t read_err = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, offset_buf, sizeof(offset_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(read_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    uint16_t offset = offset_buf[1] | offset_buf[0] << 8;
    return offset;
}
uint16_t sfm_read_scale() {
    uint8_t scale_cmd[] = { 0x30, 0xDE };
    esp_err_t write_err = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, scale_cmd, sizeof(scale_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(write_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Read the 2 byte flow scale value
    uint8_t scale_buf[2];
    esp_err_t read_err = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, scale_buf, sizeof(scale_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(read_err != ESP_OK) { return FLOW_SERIAL_INVALID; }
    // Return as 16 bit number 
    uint16_t scale = scale_buf[1] | scale_buf[0] << 8;
    return scale;
}

uint16_t sfm_read_measure() {
    // Send a start measure for every read, as recommended by Sensirion
    //sfm_start_measure();
    // Read the 2 byte raw measurement value + 1 byte CRC
    uint8_t value_buf[3];
    esp_err_t read_err = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, value_buf, sizeof(value_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(read_err != ESP_OK) { return FLOW_VALUE_NOT_AVAIL; }
    //ESP_LOGI(TAG, "Value bytes: %u %u %u", value_buf[0], value_buf[1], value_buf[2]);
    // Verify the CRC value
    if(sfm_crc(&value_buf[0], 2, value_buf[2]) != ESP_OK) { return FLOW_VALUE_INVALID_CRC; }
    // Convert to 16 bit number and return
    uint16_t value = value_buf[1] | value_buf[0] << 8;
    return value;
}
esp_err_t sfm_start_measure() {
    // Instruct the SFM to start measuring and set the register to measurement values
    uint8_t serial_cmd[] = { 0x10, 0x00 };
    return i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_cmd, sizeof(serial_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}
esp_err_t sfm_reset() {
    // Send the reset command, this also stops measurement
    uint8_t serial_cmd[] = { 0x20, 0x00 };
    return i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_cmd, sizeof(serial_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t sfm_crc(const uint8_t* buf, uint8_t buf_len, uint8_t checksum) {
    // TODO: Maybe generate table for this
    uint8_t crc = 0;
    // Calculates 8-Bit checksum with given polynomial
    for (uint8_t i = 0; i < buf_len; ++i) { 
        crc ^= (buf[i]);
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x131;
            else crc = (crc << 1);
        }
    }
    //ESP_LOGI(TAG, "CRC expected: %u Got: %u", checksum, crc);
    if (crc != checksum) return ESP_ERR_INVALID_CRC;
    else return ESP_OK;
}