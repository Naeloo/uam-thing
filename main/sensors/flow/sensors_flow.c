#include "sensors_flow.h"

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
}

void sensors_flow_start() {
    tca_select_channel(0);
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
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_cmd, sizeof(serial_cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    // Read the 4 byte serial number
    uint8_t serial_buf[4];
    ESP_ERROR_CHECK(i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SFM_ADDRESS, serial_buf, sizeof(serial_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    // Convert to 32 bit number
    uint32_t serial_no = serial_buf[0] | serial_buf[1] << 8 | serial_buf[2] << 16 | serial_buf[3] << 24;
    return serial_no;
}