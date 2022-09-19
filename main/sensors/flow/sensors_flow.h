#include "esp_log.h"
#include "driver/i2c.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define I2C_MASTER_SCL_IO           22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define I2C_TCA_ADDRESS             0x70
#define I2C_SFM_ADDRESS             0x40

#define FLOW_SENSORS_COUNT          4
#define FLOW_SERIAL_INVALID         0
#define FLOW_VALUE_INVALID_CRC      65535
#define FLOW_VALUE_NOT_AVAIL        65534

typedef struct flow_sensor {
    char id[3];
    uint8_t channel;
    uint32_t serial;
    uint16_t offset;
    uint16_t scale;
} flow_sensor;

typedef void (*sensors_flow_handler)(flow_sensor* sensor, int reading);

void sensors_flow_init();
void sensors_flow_start();
void sensors_flow_stop();

void sensors_flow_measure();
void sensors_flow_on_reading(sensors_flow_handler h);

void tca_select_channel(uint8_t channel);
uint32_t sfm_read_serial();
uint16_t sfm_read_offset();
uint16_t sfm_read_scale();
esp_err_t sfm_start_measure();
uint16_t sfm_read_measure();
esp_err_t sfm_reset();
esp_err_t sfm_crc(const uint8_t* buf, uint8_t buf_len, uint8_t checksum);