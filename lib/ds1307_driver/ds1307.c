#include "ds1307.h"
#include <stdbool.h>

static i2c_port_t i2c_port;

static uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

esp_err_t ds1307_init(const ds1307_config_t *config) {
    i2c_port = config->i2c_port;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(i2c_port, &conf);
    return i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);
}

esp_err_t ds1307_set_time(const rtc_time_t *time) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Start at register 0

    i2c_master_write_byte(cmd, dec_to_bcd(time->seconds), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->minutes), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->hours), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->day), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->date), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->month), true);
    i2c_master_write_byte(cmd, dec_to_bcd(time->year), true);

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t ds1307_get_time(rtc_time_t *time) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Start at register 0
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        return ret;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);

    i2c_master_read_byte(cmd, &time->seconds, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->minutes, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->hours, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->day, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->date, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->month, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &time->year, I2C_MASTER_NACK);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    time->seconds = bcd_to_dec(time->seconds & 0x7F); // Mask the CH bit
    time->minutes = bcd_to_dec(time->minutes);
    time->hours = bcd_to_dec(time->hours);
    time->day = bcd_to_dec(time->day);
    time->date = bcd_to_dec(time->date);
    time->month = bcd_to_dec(time->month);
    time->year = bcd_to_dec(time->year);

    return ret;
}

esp_err_t ds1307_is_running(bool *is_running) {
    uint8_t seconds;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Start at register 0
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        return ret;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &seconds, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    *is_running = !(seconds & (1 << 7));

    return ret;
}

esp_err_t ds1307_reset(void) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Start at register 0

    i2c_master_write_byte(cmd, 0x80, true); // Halt clock and clear seconds
    i2c_master_write_byte(cmd, 0x00, true); // Clear minutes
    i2c_master_write_byte(cmd, 0x00, true); // Clear hours
    i2c_master_write_byte(cmd, 0x00, true); // Clear day
    i2c_master_write_byte(cmd, 0x00, true); // Clear date
    i2c_master_write_byte(cmd, 0x00, true); // Clear month
    i2c_master_write_byte(cmd, 0x00, true); // Clear year

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
